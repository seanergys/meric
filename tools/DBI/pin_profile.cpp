/**
 * 
 * This Pintool measures runtime for functions in the application and stores 
 * list of all functions with runtime > 100 ms. 
 * List of function to measure may be specified in a file, each function 
 * on a single line, otherwise all the instrumentable functions will be measured
 *     export PIN_FILTER=fileOfFunctionsNames.txt
 * 
 * based on example from PIN repo source/tools/ManualExamples/proccount.cpp
 *
 **/

#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <cstdlib>
#include <sstream>
#include <set>
#include "pin.H"

//----------------------------------------------------------------------------//
std::set<std::string> funcList;
const std::string FILTERFILE = "PIN_FILTER";

inline std::string getenv(const std::string &param)
{
	std::string value;
	char *var = std::getenv(param.c_str());

	if (var != NULL)
	{
		std::stringstream ss(var);
		return ss.str();
	} 
	return "";
}

void processFilterFile()
{
	std::string filterFileName = getenv(FILTERFILE);
	if(filterFileName != "")
	{
		std::ifstream filterFile (filterFileName.c_str());
		std::string functionName;
		while (filterFile >> functionName)
			funcList.insert(functionName);
		filterFile.close();
	}
}
//----------------------------------------------------------------------------//

ofstream outFile;
//OS_Time give us time in microseconds, 1 milisecond is 1000 microseconds
const unsigned int MILISECOND = 1000; 


// Holds instruction count for a single procedure
typedef struct RtnCount
{
    string _name;
    string _image;
    ADDRINT _address;
    RTN _rtn;
    UINT64 _rtnCount;	//routine count
    UINT64 _icount;		//instruction count
	UINT64 lastStart;	//time
    UINT64 maxDuration;	//runtime
    UINT64 minDuration;	//runtime
    string path;
    struct RtnCount * _next;
} RTN_COUNT;

// Linked list of instruction counts for each routine
RTN_COUNT * RtnList = 0;

const char * StripPath(const char * path)
{
    const char * file = strrchr(path,'/');
    if (file)
        return file+1;
    else
        return path;
}

//----------------------------------------------------------------------------//
void startTimer(UINT64 * startTime, string * name, THREADID threadid)
{
	if (threadid == 0)
	{
		OS_Time ( startTime);
//		std::cerr << "START "+(*name)+"\n";
	}
}

void startTimerNoThrdControl(UINT64 * startTime)
{
	OS_Time ( startTime);
}

void stopTimer(UINT64 * minDuration, UINT64 * maxDuration, UINT64 * startTime, UINT64 * rtnCount, string * name, THREADID threadid)
{
	if (threadid == 0 && (*startTime) != 0)
	{
		UINT64 duration;
		OS_Time ( &duration);
		duration = duration - (*startTime);
		if (duration > (*maxDuration))
			(*maxDuration) = duration;
		if (duration < (*minDuration) || (*minDuration) == 0)
			(*minDuration) = duration;
		(*startTime) = 0;
		(*rtnCount)++;
//		std::cout << duration << std::endl;
//		std::cerr << "STOP "+(*name)+"\n";
	}
}

void stopTimerNoThrdControl(UINT64 * minDuration, UINT64 * maxDuration, UINT64 * startTime, UINT64 * rtnCount)
{
	if ((*startTime) != 0)
	{
		UINT64 duration;
		OS_Time ( &duration);
		duration = duration - (*startTime);
		if (duration > (*maxDuration))
			(*maxDuration) = duration;
		if (duration < (*minDuration) || (*minDuration) == 0)
			(*minDuration) = duration;
		(*startTime) = 0;
		(*rtnCount)++;
	}
}


// Pin calls this function every time a new rtn is executed
VOID InstrumentAllFunctions(RTN rtn, VOID *v)
{
//	if (!IMG_IsMainExecutable(SEC_Img(RTN_Sec(rtn))))
//		return;
	 
	RTN_COUNT * rc = new RTN_COUNT;
	rc->_name = RTN_Name(rtn);
	rc->_image = StripPath(IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str());
	rc->_address = RTN_Address(rtn);
	rc->_icount = 1;
	rc->_rtnCount = 0;
	rc->lastStart = 0;
	rc->maxDuration = 0;
	rc->minDuration = 0;

	INT32 line;
	PIN_GetSourceLocation(RTN_Address(rtn), NULL, &line, &(rc->path));
	if (rc->path != "")
	{
		char* buffer = (char*) malloc (sizeof(char)*(line%10+1));
		sprintf (buffer, "%d", line);
		std::string tmp (buffer);
		rc->path +=": "+tmp;
		free(buffer);
	}

	// Add to list of routines
	rc->_next = RtnList;
	RtnList = rc;

	// Insert a call at the entry and exit point of a routine
	RTN_Open(rtn);
	RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)startTimer, IARG_PTR, &(rc->lastStart), IARG_PTR, &(rc->_name), IARG_THREAD_ID, IARG_END);
	RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)stopTimer, IARG_PTR, &(rc->minDuration), IARG_PTR, &(rc->maxDuration), IARG_PTR, &(rc->lastStart), IARG_PTR, &(rc->_rtnCount), IARG_PTR, &(rc->_name), IARG_THREAD_ID, IARG_END);

	RTN_Close(rtn);
}

// Insert Pin calls before function call in the probe mode
VOID InstrumentSelectedFunctionsPROBE(IMG img, VOID *v)
{
	if (!IMG_IsMainExecutable(img))
		return;
	
	std::set<std::string>::iterator it;
	for (it = funcList.begin(); it != funcList.end(); ++it)
	{
		RTN routine = RTN_FindByName(img, (*it).c_str());
		if (RTN_Valid(routine))
		{
			RTN_COUNT * rc = new RTN_COUNT;
			rc->_name = RTN_Name(routine);
			rc->_image = StripPath(IMG_Name(SEC_Img(RTN_Sec(routine))).c_str());
			rc->_address = RTN_Address(routine);
			rc->_icount = 1;
			rc->_rtnCount = 0;
			rc->lastStart = 0;
			rc->maxDuration = 0;
			rc->minDuration = 0;
/*
            string path;
            INT32 line;
            PIN_GetSourceLocation(RTN_Address(rtn), NULL, &line, &path);
            if (path != "")
                std::cout << "File " << path << " Line " << line << std::endl; 
*/	
			// Add to the list of routines
			rc->_next = RtnList;
			RtnList = rc;

			// added call before the detected function
			// we must call NoThreadConstrol function, since support for PIN
			// threaded functions is in JIT mode only
		    RTN_InsertCallProbed(routine, IPOINT_BEFORE, (AFUNPTR)startTimerNoThrdControl, IARG_PTR, &(rc->lastStart), IARG_END);
		    
			// to insert call after the function in the PROBE mode we would need 
			// its prototype, which we do not have ...
		}
	}
}

// inserts Pin calls before and after function in the JIT mode
VOID InstrumentSelectedFunctionsJIT(IMG img, VOID *v)
{
	if (!IMG_IsMainExecutable(img))
		return;
	
	std::set<std::string>::iterator it;
	for (it = funcList.begin(); it != funcList.end(); ++it)
	{
		RTN routine = RTN_FindByName(img, (*it).c_str());
		if (RTN_Valid(routine))
		{
			RTN_COUNT * rc = new RTN_COUNT;
			rc->_name = RTN_Name(routine);
			rc->_image = StripPath(IMG_Name(SEC_Img(RTN_Sec(routine))).c_str());
			rc->_address = RTN_Address(routine);
			rc->_icount = 1;
			rc->_rtnCount = 0;
			rc->lastStart = 0;
			rc->maxDuration = 0;
			rc->minDuration = 0;
/*
            string path;
            INT32 line;
            PIN_GetSourceLocation(RTN_Address(rtn), NULL, &line, &path);
            if (path != "")
                std::cout << "File " << path << " Line " << line << std::endl; 
*/	
			// Add to the list of routines
			rc->_next = RtnList;
			RtnList = rc;

			RTN_Open(routine);
			RTN_InsertCall(routine, IPOINT_BEFORE, (AFUNPTR)startTimer, IARG_PTR, &(rc->lastStart), IARG_PTR, &(rc->_name), IARG_THREAD_ID, IARG_END);
			RTN_InsertCall(routine, IPOINT_AFTER, (AFUNPTR)stopTimer, IARG_PTR, &(rc->minDuration), IARG_PTR, &(rc->maxDuration), IARG_PTR, &(rc->lastStart), IARG_PTR, &(rc->_rtnCount), IARG_PTR, &(rc->_name), IARG_THREAD_ID, IARG_END);
			RTN_Close(routine);
		}
	}
}

// This function is called when the application exits
// It prints the name and count for each procedure
VOID Fini(INT32 code, VOID *v)
{
	outFile.open("proccount.out");
/*	std::cout << setw(16) << "Image" << " "
	          << setw(18) << "Address" << " "
	          << setw(12) << "Calls" << " "
	          << setw(12) << "Max [ms]" << " "
	          << setw(12) << "Min [ms]" << " "
	          << setw(23) << "Procedure" 
	          << std::endl
	          << "-------------------------------------------------"
	          << "-------------------------------------------------"
	          << std::endl;
*/
	int numOfSelectedRoutines = 0;
	int numOfAnalysedRoutines = 0;
	RTN_COUNT * rc;
    for (rc = RtnList; rc; rc = rc->_next)
    {	
    	numOfAnalysedRoutines++;
        if (rc->minDuration > MILISECOND*1000)
        {
/*
            std::cerr << "+" << setw(15) << rc->_image << " "
                    << setw(18) << hex << rc->_address << dec <<" "
                    << setw(12) << rc->_rtnCount << " "
                    << setw(12) << rc->maxDuration/MILISECOND<< " "
                    << setw(12) << rc->minDuration/MILISECOND<< " "
                    << setw(23) << rc->_name << " "
                    << setw(30) << rc->path
                    << std::endl;
*/
    		outFile << rc->_name << std::endl;
    		numOfSelectedRoutines++;
        }
/*        else
        	if (rc->_rtnCount > 0)
	        	std::cerr << "-" <<setw(15) << rc->_image << " "
                    << setw(18) << hex << rc->_address << dec <<" "
                    << setw(12) << rc->_rtnCount << " "
                    << setw(12) << rc->maxDuration/MILISECOND<< " "
                    << setw(12) << rc->minDuration/MILISECOND<< " "
                    << setw(23) << rc->_name 
                    << std::endl;
*/
    }
	outFile.close();
	std::cout << "Number of analysed routines: " << numOfAnalysedRoutines << std::endl
	          << "Number of selected routines: " << numOfSelectedRoutines << std::endl;

	rc = RtnList;
	while(rc)
	{
		RTN_COUNT * rcDel = rc;
		rc = rc->_next;
		free(rcDel);
	}
	
}

////////////////////////////////////////////////////////////////////////////////

INT32 printHelp()
{
    std::cerr << "This Pintool measures runtime for each function in the application" << std::endl
              << "and stores list of all functions with runtime > 100 ms" << std::endl
              << "List of function to measure may be specified in a file: " <<std::endl
              << "export PIN_FILTER=fileOfFunctionsNames.txt" << std::endl
              << std::endl << KNOB_BASE::StringKnobSummary() << std::endl;
    return -1;
}

int main(int argc, char * argv[])
{
	processFilterFile();

	// Initialize pin
	PIN_InitSymbols();
	if (PIN_Init(argc, argv))
		return printHelp();


	// Register Routine to be called to instrument rtn
/*	
	// Since we have list of functions we might use the probe mode, however 
	// the functionality is so reduced, that using JIT mode makes life easier
	if (!funcList.empty())
	{
		std::cout << "Probe IMAGE analysis\n";
		IMG_AddInstrumentFunction(InstrumentSelectedFunctionsPROBE, 0);
	
		// Register Fini to be called when the application exits
		PIN_AddFiniFunction(Fini, 0);
	
		PIN_StartProgramProbed();
	}
	else
*/
	{
		std::cout << "JIT ROUTINE analysis\n";
		if (funcList.empty())
			RTN_AddInstrumentFunction(InstrumentAllFunctions, 0);
		else // replaces the PROBED instrumentation of the binary
			IMG_AddInstrumentFunction(InstrumentSelectedFunctionsJIT, 0);

		// Register Fini to be called when the application exits
		PIN_AddFiniFunction(Fini, 0);
	
		// Start the program, never returns
		PIN_StartProgram();
	}
		
    return 0;
}

