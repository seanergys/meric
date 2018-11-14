#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>

#include <algorithm>
//#include <sstream>

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
void printHelp();
void handleParameters(int argc, char *argv[]);
void processFile(std::string fileName, std::vector<std::string> * elements);
void splitString(std::string s, const char delimiter, std::vector<std::string> * param);
template<typename TBpatch> bool insertFunction(TBpatch *appBin, BPatch_image *appImage, 
	std::vector<BPatch_function*> insertFuncs, std::vector<BPatch_snippet*> insertFuncArgs,
	const char * instrumentFuncName, BPatch_procedureLocation instrumentPoint);

BPatch_addressSpace* startDBIapplication();
void runDBIapplication(BPatch_addressSpace* app);
void readFilterFile(BPatch_image *appImage, const char * appName);
void insertTimeInstrumentation(BPatch_addressSpace *appBin, BPatch_image *appImage);

int writeBinary(BPatch_binaryEdit *binary, const char * binaryName);
bool insertMericInitAndClose(BPatch_binaryEdit *appBin, BPatch_image *appImage);
void insertMericInstrumentation(BPatch_binaryEdit *appBin, BPatch_image *appImage);
void loadSharedLibs(BPatch_binaryEdit *appBin, BPatch_binaryEdit *MPIBin = NULL);

int profileApplication();
int instrumentApplication();
int main (int argc, char *argv[]);
////////////////////////////////////////////////////////////////////////////////
BPatch bpatch;
std::vector<std::string> funcFilterList;

struct Tparametres{
	std::string input;
	std::string output;
	std::string funclist;
	std::string dependencies;
	std::string omplib;
	std::string mpilib;
	bool mpi;
	bool profile;
	bool instrument;
	uint trashold;
	std::vector <const char *> argv;
};
Tparametres params = {"", "instrumentedBin", "", "", "", "", false, false, false, 100, {}};

// SHARED FUNCTIONS ////////////////////////////////////////////////////////////

void printHelp()
{
	std::cerr << "\nThis Dyninst application providing an application binary profiling\n"
		<< "and instrumentation of selected application functions with MERIC\n"
		<< "\n"
		<< "APPLICATION USE SELECTION:\n"
		<< " At least one of following parameters must be specified\n"
		<< "\t --profile\n"
		<< "\t      = profiling of the target application\n"
		<< "\t      creates list of functions to instrument with MERIC\n"
		<< "\t --instrument\n"
		<< "\t      = instrumentation of the application with MERIC\n"
		<< "\n"
		<< "COMPULSORY PARAMETERS:\n"
		<< "\t --input=binary \n"
		<< "\t      = input binary name\n"
		<< "\t --funclist=file\n"
		<< "\t      = input file with functions to instrument, function name per line\n"
		<< "\t      for profiling this will be the output file\n"
		<< "\n"
		<< "OPTIONAL PARAMETERS - PROFILING:\n"
		<< "\t --argv [list of application parameters]\n"
		<< "\t      = this parameter must be specified as the last one, \n"
		<< "\t      all the following parameters are considered as parameter of the application we profile\n"
		<< "\t --trashold=miliseconds\n"
		<< "\t      = minimum function runtime to be added to the list of function that should be instrumented\n"
		<< "\t      default value 100ms\n"
		<< "\n"
		<< "OPTIONAL PARAMETERS - INSTRUMENTATION:\n"
		<< "\t --output=binary\n"
		<< "\t      = name of the output binary\n"
		<< "\t      if not specified the default name 'instrumentedBin' is used\n"
		<< "\t --mpi=[path/to/libmpi.so]\n"
		<< "\t      = parameter must be used for MPI applications\n"
		<< "\t      if instrumented version of libmpi.so already exists, \n"
		<< "\t      than the path to libmpi.so does not have to be specified\n"
		<< "\t --dependencies=file\n"
		<< "\t      = list of shared libraries that should be added, \n"
		<< "\t      if not specified full list of libraries is applied\n"
		<< "\t --omp=libomp.so\n"
		<< "\t      = if the instrumented binary does not use OpenMP, however MERIC with\n"
		<< "\t      OpenMP support is demand, than the libomp.so (libgomp.so; libiomp5.so; ...)\n"
		<< "\t      dependency can by added using this parameter\n\n";
	exit(1);
}

/**
 * reads commanline parametrs and sets global variable 'parameters'
 **/
void handleParameters(int argc, char *argv[])
{
	if (argc == 1)
	{
		printHelp();
	}

	for (int i = 1; i < argc; ++i)
	{
		std::vector<std::string> flag;
		splitString(std::string(argv[i]), '=', &flag);
		
		if (flag[0] == "--profile")
		{
			params.profile = true;
		}
		else if (flag[0] == "--instrument")
		{
			params.instrument = true;
		}
		else if (flag[0] == "--trashold")
		{
			if(flag.size() != 2)
				printHelp();
			//std::istringstream ss(flag[1]) >> params.trashold;
			try
			{
				params.trashold = std::stoi(flag[1]);
			}
			catch (...)
			{
				printHelp();
			}
		}
		else if (flag[0] == "--input")
		{
			if(flag.size() != 2)
				printHelp();
			params.input = flag[1];
		}
		else if (flag[0] == "--output")
		{
			if(flag.size() != 2)
				printHelp();
			params.output = flag[1];
		}
		else if (flag[0] == "--funclist")
		{
			if(flag.size() != 2)
				printHelp();
			params.funclist = flag[1];
		}
		else if (flag[0] == "--mpi")
		{
			params.mpi=true;
			if(flag.size() == 2)
				params.mpilib = flag[1];
		}
		else if (flag[0] == "--dependencies")
		{
			if(flag.size() != 2)
				printHelp();
			params.dependencies = flag[1];
		}
		else if (flag[0] == "--omp")
		{
			if(flag.size() != 2)
				printHelp();
			params.omplib = flag[1];
		}
		else if (flag[0] == "--argv")
		{
			if(params.input == "")
				printHelp();
			params.argv.push_back(params.input.c_str());
			for (int j = i; j < argc; ++j)
			{
				params.argv.push_back(argv[j]);
			}
		}else
		{
			printHelp();
		}
	}

	if (params.input == "" || params.funclist == "" || (!params.profile && !params.instrument)) //compulsory
		printHelp();
}

/**
 * reads a file with input information
 * - list of functions to instrument
 * - list of shared libraries to load
 **/
void processFile(std::string fileName, std::vector<std::string> * elements)
{
	if(fileName != "")
	{
		std::ifstream inputfile (fileName.c_str());
		std::string element;
		while (inputfile >> element)
			(*elements).push_back(element);
		inputfile.close();
	}
}

/**
 * split a string according delimiter a returns them in vector
 **/
void splitString(std::string s, const char delimiter, std::vector<std::string> * param)
{
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos)
	{
		(*param).push_back(s.substr(0, pos));
		s.erase(0, pos + 1);
	}
	(*param).push_back(s);
}

/**
 * Insert function into binary berore or after another function exection
 * inserted function must be found in the binary before calling this function 
 * if it would be searched here the work would be redundant (case of MERIC_MeasureStart/Stop)
 **/
//BPatch_addressSpace
//BPatch_binaryEdit 
template<typename TBpatch>
bool insertFunction(TBpatch *appBin, BPatch_image *appImage,
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

// PROFILE /////////////////////////////////////////////////////////////////////

/**
 * creates a new process and return its handle
 **/
BPatch_addressSpace* startDBIapplication()
{
	BPatch_addressSpace* appAddrSpace = bpatch.processCreate(params.input.c_str(), &(params.argv[0]));
	if (!appAddrSpace)
	{
		std::cerr << "processCreate failed\n";
		exit(1);
	}

	if (!appAddrSpace->loadLibrary("libtimeprofile.so"))
	{
		std::cerr << "failed to load libtimeprofile.so" << std::endl;
		exit(1);
	}
	return appAddrSpace;
}

/**
 * runs the stopped process and waits until it ends
 **/
void runDBIapplication(BPatch_addressSpace* app)
{
	BPatch_process* appProc = dynamic_cast<BPatch_process*>(app);
	if (appProc)
	{
		if (!appProc->continueExecution())
			std::cerr << "continueExecution failed\n";
		while (!appProc->isTerminated())
		{
			bpatch.waitForStatusChange();
		}
	}
}

/**
 * reads a file with names of functions to instrument
 * if the file does not exists or is empty than we instrument all available functions
 **/
void readFilterFile(BPatch_image *appImage, const char * appName)
{
	funcFilterList.clear();
	processFile(params.funclist, &funcFilterList);
	if (funcFilterList.size() == 0)
	{
		std::vector < BPatch_function * >*allFunctions;
/*		//searching not only in the main application module
		if (deepSearch)
		{
			allFunctions = appImage->getProcedures(false);
		}
		else
*/
		{
			BPatch_module * mainModule = appImage->findModule(appName, false);
			allFunctions = mainModule->getProcedures(false);
		}

		std::vector < BPatch_function * >::iterator funcIter;
		for (funcIter = allFunctions->begin (); funcIter != allFunctions->end (); ++funcIter)
		{
			BPatch_function *curFunc = *funcIter;
			funcFilterList.push_back(curFunc->getDemangledName());
		}
	}
}

/**
 * instrumentation of the selected functions to measure their runtime
 **/
void insertTimeInstrumentation(BPatch_addressSpace *appBin, BPatch_image *appImage)
{
//	std::cerr << "Inserting TIMEPROFILE instrumentation for following application functions\n";

	std::vector<BPatch_function*> startFuncs;
	std::vector<BPatch_function*> stopFuncs;
	appImage->findFunction("TIMEPROFILE_regionStart", startFuncs, true, true, true);
	appImage->findFunction("TIMEPROFILE_regionStop", stopFuncs, true, true, true);
	if (startFuncs.size() == 0)
	{
		std::cerr << "Could not find TIMEPROFILE_regionStart function\n";
		return;
	}
	if (stopFuncs.size() == 0)
	{
		std::cerr << "Could not find TIMEPROFILE_regionStop function\n";
		return;
	}
	
	//Instrument functions from the list
	for (auto funcName : funcFilterList)
	{
		std::cout << "\t" << funcName << std::endl;

		std::vector<BPatch_snippet*> funcArgs;
		BPatch_snippet* param = new BPatch_constExpr(funcName.c_str());
		funcArgs.push_back(param);		
		if (insertFunction<BPatch_addressSpace>(appBin, appImage, startFuncs, funcArgs, funcName.c_str(), BPatch_entry))
			//TODO end execution in case of function call insertion fail
			insertFunction<BPatch_addressSpace>(appBin, appImage, stopFuncs,  funcArgs,  funcName.c_str(), BPatch_exit);
	}

// TIMEPROFILE_evaluate	TODO add checking
	std::vector<BPatch_function*> evaluateFuncs;
	appImage->findFunction("TIMEPROFILE_evaluate", evaluateFuncs, true, true, true);
	if(evaluateFuncs.size() == 0)
	{
		std::cerr << "Could not find TIMEPROFILE_evaluate function\n";
		return;
	}
	std::vector<BPatch_snippet*> evaluateArgs;
	BPatch_snippet* param1 = new BPatch_constExpr(params.funclist.c_str());	//LogFile TODO <<<<<<<<<<<<<<< toto muze delat problemy <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	BPatch_snippet* param2 = new BPatch_constExpr(params.trashold);	//TRASHOLD
	evaluateArgs.push_back(param1);
	evaluateArgs.push_back(param2);
	insertFunction<BPatch_addressSpace>(appBin, appImage, evaluateFuncs, evaluateArgs, "main", BPatch_exit);
}

// INSTRUMENT //////////////////////////////////////////////////////////////////

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
	appImage->findFunction("MERIC_Init", initFuncs, true, true, true);
	appImage->findFunction("MERIC_Close", closeFuncs, true, true, true);
	if (initFuncs.size() == 0 || closeFuncs.size() == 0)
	{
		std::cerr<< "Could not find MERIC_Init/Close function\n";
		return false;
	}

	if(params.mpi)
	{
		if (insertFunction<BPatch_binaryEdit>(appBin, appImage, initFuncs, mericArgs, "MPI_Init", BPatch_exit) &&
		insertFunction<BPatch_binaryEdit>(appBin, appImage, closeFuncs, mericArgs, "MPI_Finalize", BPatch_entry))
			return true;
	}
	else
	{
		if (insertFunction<BPatch_binaryEdit>(appBin, appImage, initFuncs, mericArgs, "main", BPatch_entry) &&
		insertFunction<BPatch_binaryEdit>(appBin, appImage, closeFuncs, mericArgs, "main", BPatch_exit))
			return true;
	}
	return false;
}

/**
 * Instrumentation of the selected function
 **/
void insertMericInstrumentation(BPatch_binaryEdit *appBin, BPatch_image *appImage)
{
	std::cerr << "Inserting MERIC instrumentation for following application functions\n";

	std::vector<BPatch_function*> startFuncs;
	std::vector<BPatch_function*> stopFuncs;
	appImage->findFunction("MERIC_MeasureStart", startFuncs, true, true, true);
	appImage->findFunction("MERIC_MeasureStop", stopFuncs, true, true, true);
	if (startFuncs.size() == 0)
	{
		std::cerr << "Could not find MERIC_MeasureStart function\n";
		return;
	}
	if (stopFuncs.size() == 0)
	{
		std::cerr << "Could not find MERIC_MeasureStop function\n";
		return;
	}
	
	//Instrument functions from the list
	std::vector<BPatch_snippet*> stopArgs; //always empty
	for (auto funcName : funcFilterList)
	{
		std::cout << "\t" << funcName << std::endl;

		std::vector<BPatch_snippet*> startArgs;
		BPatch_snippet* param = new BPatch_constExpr(funcName.c_str());
		startArgs.push_back(param);		
		if (insertFunction<BPatch_binaryEdit>(appBin, appImage, startFuncs, startArgs, funcName.c_str(), BPatch_entry))
			//TODO end execution in case of function call insertion fail
			insertFunction<BPatch_binaryEdit>(appBin, appImage, stopFuncs,  stopArgs,  funcName.c_str(), BPatch_exit);
	}
}

/**
 * load necessary shared libraries to the binary
 * if we instrument (Init and Close) libmpi.so we must add them all too
 **/
void loadSharedLibs(BPatch_binaryEdit *appBin, BPatch_binaryEdit *MPIBin)
{
	std::vector<std::string> extraSharedLibraries;
	if (params.dependencies != "")
	{
		processFile(params.dependencies, &extraSharedLibraries);
	}
	else
	{
		if (params.mpi)
			extraSharedLibraries.push_back("libmericmpi.so");
		else
			extraSharedLibraries.push_back("libmeric.so");
		extraSharedLibraries.push_back("libx86_adapt.so");
		extraSharedLibraries.push_back("librt.so");
		extraSharedLibraries.push_back("libpapi.so");
		extraSharedLibraries.push_back("libnuma.so");
		extraSharedLibraries.push_back("libhdeem.so");
		extraSharedLibraries.push_back("libfreeipmi.so");
		extraSharedLibraries.push_back("libcpufreq.so");
		extraSharedLibraries.push_back("libpthread.so");	//so-far always N/A
		//GCC -fopenmp	libgomp.so
		//ICC -qopenmp	libiomp5.so	
		//extraSharedLibraries.push_back("libgomp.so");
		//extraSharedLibraries.push_back("libiomp5.so");
		if (params.omplib != "")
			extraSharedLibraries.push_back(params.omplib.c_str());
	}

	std::cerr << "Inserting shared libraries dependencies - APP\n";
	for (auto libName : extraSharedLibraries)
	{
		if (appBin->loadLibrary(libName.c_str()))
		{
			if (MPIBin != NULL)
				MPIBin->loadLibrary(libName.c_str());
			std::cerr << "\tOK \t" << libName << std::endl;
		}
		else
			std::cerr << "\tN/A\t" << libName << std::endl;
	}
}

// MAIN FUNCTIONS //////////////////////////////////////////////////////////////

int profileApplication()
{
	BPatch_addressSpace *appAddrSpace = NULL;
	BPatch_image *appImage = NULL;
	
	std::vector<std::string> appParams;
	splitString(params.input, '/', &appParams);

	readFilterFile(appImage, appParams.back().c_str());

	// running the application to detect funcitions with runtime > trashold
	// due to removing instrumentation the overhead will be reduced and duration 
	// of some functions may drop under the trashold
	// run the application until we will have persistent list of functions
	size_t prevFuncListSize = 0;
	size_t i=0;
	while(funcFilterList.size() > 0 && funcFilterList.size() < prevFuncListSize)
	{
		std::cerr << "Profiling - iteration#" << ++i << std::endl;
		
		prevFuncListSize = funcFilterList.size();
		appAddrSpace = startDBIapplication();
		appImage = appAddrSpace->getImage();
		insertTimeInstrumentation(appAddrSpace, appImage);
		runDBIapplication(appAddrSpace);
		readFilterFile(appImage, appParams.back().c_str());
	}

	return 0;
}

int instrumentApplication()
{
	processFile(params.funclist, &funcFilterList);
	if (funcFilterList.empty())
	{
		std::cerr << "List of functions to instrument with MERIC is empty\n";
		return EXIT_FAILURE;
	}
	
	BPatch_binaryEdit *appBin = bpatch.openBinary (params.input.c_str(), false);
	if (appBin == NULL)
	{
		std::cerr << "Failed to open binary" << std::endl;
		return EXIT_FAILURE;
	}
	
	BPatch_image *MPIImage;
	BPatch_binaryEdit *MPIBin;
	if (params.mpilib != "")
	{
		MPIBin = bpatch.openBinary (params.mpilib.c_str(), false);
		if (MPIBin == NULL)
		{
			std::cerr << "Failed to open MPI binary" << std::endl;
			return EXIT_FAILURE;
		}
		MPIImage = MPIBin->getImage();
		loadSharedLibs(appBin, MPIBin);
	}
	else
		loadSharedLibs(appBin);

	BPatch_image *appImage = appBin->getImage();
	if (params.mpi)
	{
		if (params.mpilib != "")
		{
			if(insertMericInitAndClose(MPIBin, MPIImage))
			{
				if (writeBinary(MPIBin, "libmpi.so"))
					return EXIT_FAILURE;
			}
		}
	}
	else
	{
		if(!insertMericInitAndClose(appBin, appImage))
			return EXIT_FAILURE;
	}
	
	insertMericInstrumentation(appBin, appImage);

	// Output the instrumented binary
	if (!writeBinary(appBin, params.output.c_str()))
	{
		std::cerr << "Static Binary Instrumentation successfully finished\n";
		if(params.mpi)
			std::cerr << "To use instrumented version of MPI, run the application as follows:\n"
			          << "LD_PRELOAD=$(pwd)/libmpi.so mpirun -n $NUMPROC ./"<< params.output <<" [APP_PARAMS]\n\n";
		return 0;
	}
	return 1;
}

int main (int argc, char *argv[])
{
	handleParameters(argc, argv);

	int retVal = 0;
	if (params.profile)
		retVal = profileApplication();

	if (params.instrument && !retVal)
		retVal = instrumentApplication();

	return retVal;
}





