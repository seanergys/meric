#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>

// DyninstAPI includes
#include "BPatch.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_flowGraph.h"
#include "BPatch_function.h"
#include "BPatch_point.h"
#include "BPatch_object.h"
#include "BPatch_module.h"

#include "BPatch_addressSpace.h"
#include "BPatch_process.h"

////////////////////////////////////////////////////////////////////////////////
bool MPI=true;
////////////////////////////////////////////////////////////////////////////////
char *inBinary = NULL;
BPatch bpatch;
std::set<std::string> funcFilterList;
const std::string FILTERFILE = "DINST_FILTER";

////////////////////////////////////////////////////////////////////////////////

/**
 * reads environment variables
 **/
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

/**
 * reads a list of functions from the input file to instrument
 **/
void processFilterFile()
{
	std::string filterFileName = getenv(FILTERFILE);
	if(filterFileName != "")
	{
		std::ifstream filterFile (filterFileName.c_str());
		std::string functionName;
		while (filterFile >> functionName)
			funcFilterList.insert(functionName);
		filterFile.close();
	}
}

/**
 * Writes new edited version of the binary
 **/
int writeBinary(BPatch_binaryEdit *binary, const char * binaryName)
{
	std::cerr << "NEW binary writing - "<< binaryName << std::endl;
	if (!binary->writeFile (binaryName))
	{
		std::cerr << "Failed to write output binary\n";
		return 1;
	}
	return 0;
}

/**
 * Insert function into binary berore or after another function exection
 * inserted function must be found in the binary before calling this function 
 * if it would be searched here the work would be redundant (case of MERIC_MeasureStart/Stop)
 **/
bool insertFunction(BPatch_binaryEdit *appBin, BPatch_image *appImage, 
	//parameters of inserted function
	std::vector<BPatch_function*> insertFuncs, std::vector<BPatch_snippet*> insertFuncArgs,
	//parameters of function that is executed before/after inserted function 
	const char * instrumentFuncName, BPatch_procedureLocation instrumentPoint)
{
	std::vector<BPatch_function*> functions;
	std::vector<BPatch_point *> *points;
	
	appImage->findFunction(instrumentFuncName, functions);
	if (functions.size() == 0)
	{
		std::cerr << "No "<< instrumentFuncName <<" to instrument\n";
		return false;
	}
	
	points = functions[0]->findPoint(instrumentPoint);
	if (!points || points->size() == 0)
	{
		std::cerr << "No points for " << instrumentFuncName << std::endl;
		return false;
	}

	BPatch_funcCallExpr insertCall(*(insertFuncs[0]), insertFuncArgs);
	if (!appBin->insertSnippet(insertCall, *points))
	{
		std::cerr << "insertSnippet for "<< instrumentFuncName <<" failed\n";
		return false;
	}
	return true;
}

/**
 * Insert MERIC_Init() and MERIC_Close() 
 * The functions should be the first and the last function calls in the application
 * In case of MPI application Init follows MPI_Init and Close precedes MPI_Finalize
 **/
bool insertMericInitAndClose(BPatch_binaryEdit *appBin, BPatch_image *appImage)
{
	std::vector<BPatch_function*> functions;
	std::vector<BPatch_snippet*> mericArgs; //always empty

	std::vector<BPatch_function*> initFuncs;
	std::vector<BPatch_function*> closeFuncs;
	appImage->findFunction("MERIC_Init", initFuncs);
	appImage->findFunction("MERIC_Close", closeFuncs);
	if (initFuncs.size() == 0 || closeFuncs.size() == 0)
	{
		std::cerr<< "Could not find MERIC_Init/Close function\n";
		return false;
	}

	if(MPI)
	{
		if (insertFunction(appBin, appImage, initFuncs, mericArgs, "MPI_Init", BPatch_exit) &&
		insertFunction(appBin, appImage, closeFuncs, mericArgs, "MPI_Finalize", BPatch_entry))
			return true;
	}
	else
	{
		if (insertFunction(appBin, appImage, initFuncs, mericArgs, "main", BPatch_entry) &&
		insertFunction(appBin, appImage, closeFuncs, mericArgs, "main", BPatch_exit))
			return true;
	}
	return false;

////////////////////////////////////////////////////////////////////////////////

/*
	std::vector<BPatch_point *> *points;

	const char * InitFunction;	//function related with MERIC_Init
	const char * CloseFunction;	//function related with MERIC_Close
	BPatch_procedureLocation InitPoint; //MERIC_Init insertion in InitPoint of InitFunction
	BPatch_procedureLocation ClosePoint; //MERIC_Close insertion in ClosePoint of CloseFunction

	if (MPI)
	{
		InitFunction = "MPI_Init";
		InitPoint = BPatch_exit;
		CloseFunction = "MPI_Finalize";
		ClosePoint = BPatch_entry;
	}
	else
	{
		InitFunction = "main";
		InitPoint = BPatch_entry;
		CloseFunction = "main";
		ClosePoint = BPatch_exit;
	}

	
	//Insert MERIC_Init
	appImage->findFunction(InitFunction, functions);
	if (functions.size() == 0)
	{
		std::cerr << "No "<<InitFunction <<" to instrument\n";
		return false;
	}
	points = functions[0]->findPoint(InitPoint);
	if (!points || points->size() == 0)
	{
		std::cerr << "No entry points for " << InitFunction << std::endl;
		return false;
	}

	std::vector<BPatch_function*> initFuncs;
	appImage->findFunction("MERIC_Init", initFuncs);
	if (initFuncs.size() == 0)
	{
		std::cerr<< "Could not find MERIC_Init\n";
		return false;
	}
	else
	{
		BPatch_funcCallExpr initCall(*(initFuncs[0]), mericArgs);
		if (!appBin->insertSnippet(initCall, *points))
		{
			std::cerr << "insertSnippet for MERIC_Init failed\n";
			return false;
		}
	}

	//Insert MERIC_Close
	appImage->findFunction(CloseFunction, functions);
	if (functions.size() == 0)
	{
		std::cerr << "No "<<CloseFunction <<" to instrument\n";
		return false;
	}
	points = functions[0]->findPoint(ClosePoint);
	if (!points || points->size() == 0)
	{
		std::cerr << "No exit points for " << ClosePoint << std::endl;	//TODO
		return false;
	}

	std::vector<BPatch_function*> closeFuncs;
	appImage->findFunction("MERIC_Close", closeFuncs);
	if (closeFuncs.size() == 0)
	{
		std::cerr<< "Could not find MERIC_Close\n";
		return false;
	}
	else
	{
		BPatch_funcCallExpr closeCall(*(closeFuncs[0]), mericArgs);
		if (!appBin->insertSnippet(closeCall, *points))
		{
			std::cerr << "insertSnippet for MERIC_Close failed\n";
			return false;
		}
	}
	return true;
*/
}


/**
 * Instrumentation of the selected function
 **/
void insertMericInstrumentation(BPatch_binaryEdit *appBin, BPatch_image *appImage)
{
	std::cerr << "Inserting MERIC instrumentation for following application functions\n";

	std::vector<BPatch_function*> startFuncs;
	std::vector<BPatch_function*> stopFuncs;
	appImage->findFunction("MERIC_MeasureStart", startFuncs);
	appImage->findFunction("MERIC_MeasureStop", stopFuncs);
	if (startFuncs.size() == 0 || stopFuncs.size() == 0)
	{
		std::cerr<< "Could not find MERIC_MeasureStart/Stop function\n";
		return;
	}
	
	//Instrument functions from the list
	std::vector<BPatch_snippet*> stopArgs; //always empty
	for (std::string funcName : funcFilterList)
	{
		std::cout << "\t" << funcName << std::endl;


		std::vector<BPatch_snippet*> startArgs;
		BPatch_snippet* param = new BPatch_constExpr(funcName.c_str());
		startArgs.push_back(param);		
		if (insertFunction(appBin, appImage, startFuncs, startArgs, funcName.c_str(), BPatch_entry))
			//TODO end execution in case of function call insertion fail
			insertFunction(appBin, appImage, stopFuncs,  stopArgs,  funcName.c_str(), BPatch_exit);

/*
		std::vector<BPatch_function*> functions;
		appImage->findFunction(funcName.c_str(), functions);
		if (functions.size() == 0)
		{
			std::cerr << "Function " << funcName <<" not found!\n";
			continue;
		}


		//Insert MERIC_MeasureStart(funcName)
		std::vector<BPatch_point *> *entryPoints;
		entryPoints = functions[0]->findPoint(BPatch_entry);
		if (!entryPoints || entryPoints->size() == 0)
		{
			std::cerr << "No entry points for " << funcName << std::endl;
			continue;
		}
		
		std::vector<BPatch_snippet*> startArgs;
		BPatch_snippet* param = new BPatch_constExpr(funcName.c_str());
		startArgs.push_back(param);

		BPatch_funcCallExpr startCall(*(startFuncs[0]), startArgs);
		if (!appBin->insertSnippet(startCall, *entryPoints))
		{
			std::cerr << "insertSnippet "<< funcName <<" failed\n";
			continue;
		}

		//Insert MERIC_MeasureStop()
		std::vector<BPatch_point *> *exitPoints;
		exitPoints = functions[0]->findPoint(BPatch_exit);
		if (!exitPoints || exitPoints->size() == 0)
		{
			std::cerr << "No exit points for " << funcName << std::endl;
			continue;	//TODO nyni vlozen start ale stop selhal takze vysledna aplikace nebude fungovat
		}
		
		std::vector<BPatch_snippet*> stopArgs;

		BPatch_funcCallExpr stopCall(*(stopFuncs[0]), stopArgs);
		if (!appBin->insertSnippet(stopCall, *exitPoints))
		{
			std::cerr << "insertSnippet "<< funcName <<" failed\n";
			continue;	//TODO nyni vlozen start ale stop selhal takze vysledna aplikace nebude fungovat
		}
*/
	}
}

void printHelp()
{
	std::cerr << "This Dyninst application instruments application functions in the provided binary with MERIC functions\n"
	          << "TEMPORARY SOLUTION how to control the appliaction\n"
   	          << "Application takes one argument - name of the binary to analyse\n"
   	          << "Functions to instrument must be listed in a file, function per line\n"
   	          << "export DINST_FILTER=NameOfFileWithFunctionsToInstrument\n"
	          << "\n";
/*
//	TODO

COMPULSORY:
 --input binary	
 		= input binary name
 --funclist file
 		= input file with functions to instrument, function per line

OPTIONAL:
 --output binary
 		= name of the output binary
 --mpi [path/to/libmpi.so]
 		= parameter must be used for mpi applications
 		if instrumneted version of libmpi.so already exists, 
 		than the path to libmpi.so does not have to be added
 --dependencies	file
 		= list of shared libraries that should be added, 
 		if not specified full list of libraries is applied

//TODO OpenMP, Intel, mangeld names
*/
}

int main (int argc, char *argv[])
{

	if (argc != 2 || std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")
	{
		printHelp();
		return EXIT_FAILURE;
	}


	//TODO read name of the input and output binary from command line
	//inBinary = "../../test/testClean_mpi";	//input binary name
	char * MPIlib = "/apps/all/OpenMPI/1.10.7-GCC-7.1.0-2.28/lib/libmpi.so.12";

	/* Open the specified binary for binary rewriting. 
	* When the second parameter is set to true, all the library dependencies 
	* as well as the binary are opened */
	BPatch_binaryEdit *appBin = bpatch.openBinary (argv[1], false);
	if (appBin == NULL)
	{
		std::cerr << "Failed to open binary" << std::endl;
		return EXIT_FAILURE;
	}
	
	std::vector<const char*> extraSharedLibraries;
	
	BPatch_binaryEdit *MPIBin;
	BPatch_image *MPIImage;
	if (MPI)
	{
		extraSharedLibraries.push_back("libmericmpi.so");

		MPIBin = bpatch.openBinary (MPIlib, false);
		if (MPIBin == NULL)
		{
			std::cerr << "Failed to open MPI binary" << std::endl;
			return EXIT_FAILURE;
		}
		MPIImage = MPIBin->getImage();
	}
	else
	{
		extraSharedLibraries.push_back("libmeric.so");
	}
	
	extraSharedLibraries.push_back("libx86_adapt.so");
	extraSharedLibraries.push_back("librt.so");
	extraSharedLibraries.push_back("libpapi.so");
	extraSharedLibraries.push_back("libnuma.so");
	extraSharedLibraries.push_back("libhdeem.so");
	extraSharedLibraries.push_back("libfreeipmi.so");
	extraSharedLibraries.push_back("libcpufreq.so");

	//TODO openMP
	//GCC -fopenmp	libgomp.so
	//ICC -qopenmp	libiomp5.so	
	extraSharedLibraries.push_back("libgomp.so");
	extraSharedLibraries.push_back("libpthread.so");	//so-far always N/A

	
	//TODO get dynamically a list of necessary shares libraries that should be added to the binary
	std::cerr << "Inserting shared libraries dependencies - APP\n";
	for (auto libName : extraSharedLibraries)
	{
		if (appBin->loadLibrary(libName))
		{
			if (MPI)
				MPIBin->loadLibrary(libName);
			std::cerr << "\tOK \t" << libName << std::endl;
		}
		else
			std::cerr << "\tN/A\t" << libName << std::endl;


	}

	BPatch_image *appImage = appBin->getImage();

	if (MPI)
	{
		if(insertMericInitAndClose(MPIBin, MPIImage))
		{
			if (!writeBinary(MPIBin, "libmpi.so"))
				return EXIT_FAILURE;
		}
	}
	else
	{
		if(!insertMericInitAndClose(appBin, appImage))
			return EXIT_FAILURE;
	}
	
	processFilterFile();
	insertMericInstrumentation(appBin, appImage);

	// Output the instrumented binary
	if (writeBinary(appBin, "newTest"))
	{
		std::cerr << "Static Binary Instrumentation successfully finished\n";
		if(MPI)
			std::cerr << "To use instrumented version of MPI, run the application as follows:\n"
			          << "LD_PRELOAD=$(pwd)/libmpi.so mpirun -n $NUMPROC ./newTest [APP_PARAMS]\n\n";
		return 0;
	}
	return EXIT_FAILURE;
}





















