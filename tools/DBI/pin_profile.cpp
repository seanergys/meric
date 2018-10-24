/*
make PIN_ROOT=$PIN_PATH obj-intel64/TestPinTool.so

g++ -E -g -Wall -Werror -Wno-unknown-pragmas -D__PIN__=1 -DPIN_CRT=1 -fno-stack-protector -fno-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-rtti -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX -fabi-version=2  -I/home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/source/include/pin -I/home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/source/include/pin/gen -isystem /home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/extras/stlport/include -isystem /home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/extras/libstdc++/include -isystem /home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/extras/crt/include -isystem /home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/extras/crt/include/arch-x86_64 -isystem /home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/extras/crt/include/kernel/uapi -isystem /home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/extras/crt/include/kernel/uapi/asm-x86 -I/home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/extras/components/include -I/home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/extras/xed-intel64/include/xed -I/home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/source/tools/InstLib -O3 -fomit-frame-pointer -fno-strict-aliasing   -c -o obj-intel64/TestPinTool.o TestPinTool.cpp
g++ -E -shared -Wl,--hash-style=sysv /home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/intel64/runtime/pincrt/crtbeginS.o -Wl,-Bsymbolic -Wl,--version-script=/home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/source/include/pin/pintool.ver -fabi-version=2    -o obj-intel64/TestPinTool.so obj-intel64/TestPinTool.o  -L/home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/intel64/runtime/pincrt -L/home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/intel64/lib -L/home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/intel64/lib-ext -L/home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/extras/xed-intel64/lib -lpin -lxed /home/andrew/Software/pin-3.7-97619-g0d0c92f4f-gcc-linux/intel64/runtime/pincrt/crtendS.o -lpin3dwarf  -ldl-dynamic -nostdlib -lstlport-dynamic -lm-dynamic -lc-dynamic -lunwind-dynamic

pin -t obj-intel64/TestPinTool.so -- ./test

*/

/*
  1) GCC/4.4.7-system   2) OpenMPI/1.8.6-GCC-4.4.7-system
Could not exec /home/vys0053/Software/pin-2.14-71313-gcc.4.4.7-linux/ia32/bin/pinbin
in file Source/pin/injector_nonmac/vm_elf_loader.cpp:GenerateMiniloaderProgramForImageLoad:702

In any case for PIn to find its libraries one need to set PIN_VM32_LD_LIBRARY_PATH or PIN_VM64_LD_LIBRARY_PATH
*/


//
// based od source/tools/ManualExamples/proccount.cpp
//

#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string.h>
	#include <stdlib.h>
	#include <cstdlib>
	#include <sstream>
#include "pin.H"

//----------------------------------------------------------------------------//
#include <set>
std::set<std::string> funcList;
const std::string FILTERFILE = "PIN_FILTER";

inline std::string getenv(const std::string &param) //, std::string defaultValue)
{
	std::string value;
	char *var = std::getenv(param.c_str());

	if (var != NULL)
	{
		std::stringstream ss(var);
		return ss.str();
	} 
	return "";//defaultValue;
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
VOID Routine(RTN rtn, VOID *v)
{
//	if (!IMG_IsMainExecutable(SEC_Img(RTN_Sec(rtn))))
//		return;
	
	//if a function is not in our filter list
//	if (!funcList.empty() && funcList.find(RTN_Name(rtn)) == funcList.end())
//		return;
    
    // Allocate a counter for this routine
    RTN_COUNT * rc = new RTN_COUNT;

    // The RTN goes away when the image is unloaded, so save it now
    // because we need it in the fini
    rc->_name = RTN_Name(rtn);
    rc->_image = StripPath(IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str());
    rc->_address = RTN_Address(rtn);
    rc->_icount = 1;
    rc->_rtnCount = 0;
	rc->lastStart = 0;
	rc->maxDuration = 0;
	rc->minDuration = 0;
//	rc->path = "";

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
            
    RTN_Open(rtn);
            
    // Insert a call at the entry and exit point of a routine to increment the call count
	RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)startTimer, IARG_PTR, &(rc->lastStart), IARG_PTR, &(rc->_name), IARG_THREAD_ID, IARG_END);
	RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)stopTimer, IARG_PTR, &(rc->minDuration), IARG_PTR, &(rc->maxDuration), IARG_PTR, &(rc->lastStart), IARG_PTR, &(rc->_rtnCount), IARG_PTR, &(rc->_name), IARG_THREAD_ID, IARG_END);
	
    RTN_Close(rtn);
}


VOID ImageLoad(IMG img, VOID *v)
{
	if (!IMG_IsMainExecutable(img))
		return;
	
	std::set<std::string>::iterator it;
	for (it = funcList.begin(); it != funcList.end(); ++it)
	{
		RTN routine = RTN_FindByName(img, (*it).c_str());
		if (RTN_Valid(routine))
		{
			// Allocate a counter for this routine
			RTN_COUNT * rc = new RTN_COUNT;

			// The RTN goes away when the image is unloaded, so save it now
			// because we need it in the fini
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
                std::cout << "File " << path << " Line " << line << endl; 
*/	
			// Add to list of routines
			rc->_next = RtnList;
			RtnList = rc;

			// added call before the detected function
		    RTN_InsertCallProbed(routine, IPOINT_BEFORE, (AFUNPTR)startTimerNoThrdControl, IARG_PTR, &(rc->lastStart), IARG_END);
		    
			// call after the wrapped function requires prototype - missing in the following command
//			RTN_InsertCallProbed(routine, IPOINT_AFTER, (AFUNPTR)stopTimerNoThrdControl, IARG_PTR, &(rc->minDuration), IARG_PTR, &(rc->maxDuration), IARG_PTR, &(rc->lastStart), IARG_PTR, &(rc->_rtnCount), IARG_END);


/*			//JIT version that allows us easily insert call before and also after the function - without wrapped function prototype
			RTN_Open(routine);
			RTN_InsertCall(routine, IPOINT_BEFORE, (AFUNPTR)startTimer, IARG_PTR, &(rc->lastStart), IARG_PTR, &(rc->_name), IARG_THREAD_ID, IARG_END);
			RTN_InsertCall(routine, IPOINT_AFTER, (AFUNPTR)stopTimer, IARG_PTR, &(rc->minDuration), IARG_PTR, &(rc->maxDuration), IARG_PTR, &(rc->lastStart), IARG_PTR, &(rc->_rtnCount), IARG_PTR, &(rc->_name), IARG_THREAD_ID, IARG_END);
			RTN_Close(routine);
*/
		}
	}
}

// This function is called when the application exits
// It prints the name and count for each procedure
VOID Fini(INT32 code, VOID *v)
{
	outFile.open("proccount.out");
/*    std::cout << setw(16) << "Image" << " "
            << setw(18) << "Address" << " "
            << setw(12) << "Calls" << " "
            << setw(12) << "Max [ms]" << " "
            << setw(12) << "Min [ms]" << " "
            << setw(23) << "Procedure" 
            << endl
            << "-------------------------------------------------"
            << "-------------------------------------------------"
            << endl;
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
                    << endl;
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
                    << endl;
*/
    }
	outFile.close();
	std::cout << "Number of analysed routines: " << numOfAnalysedRoutines << std::endl;
	std::cout << "Number of selected routines: " << numOfSelectedRoutines << std::endl;

	rc = RtnList;
	while(rc)
	{
		RTN_COUNT * rcDel = rc;
		rc = rc->_next;
		free(rcDel);
	}
	
}

/* ========================================================================== */
/* Print Help Message                                                         */
/* ========================================================================== */

INT32 Usage()
{
    cerr << "This Pintool measures rintime for each function in the application" << endl;
    cerr << "and stores list of all funstions with runtime > 100 ms" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ========================================================================== */
/* Main                                                                       */
/* ========================================================================== */

int main(int argc, char * argv[])
{

	cerr << PIN_GetPid() << " - " << getpid() << std::endl;
	
	processFilterFile();

	// Initialize symbol table code, needed for rtn instrumentation
	PIN_InitSymbols();

	// Initialize pin
	if (PIN_Init(argc, argv))
		return Usage();


	// Register Routine to be called to instrument rtn
	if (funcList.empty())
	{
		std::cout << "JIT ROUTINE analysis\n";
		RTN_AddInstrumentFunction(Routine, 0);

		// Register Fini to be called when the application exits
		PIN_AddFiniFunction(Fini, 0);
	
		// Start the program, never returns
		PIN_StartProgram();
	}
	else
	{
		std::cout << "Probe IMAGE analysis\n";
		IMG_AddInstrumentFunction(ImageLoad, 0);
	
		// Register Fini to be called when the application exits
		PIN_AddFiniFunction(Fini, 0);
	
//		PIN_StartProgramProbed();
		PIN_StartProgram();
	}
	
    return 0;
}

