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
int writeBinary(BPatch_binaryEdit *binary, const char * binaryName);
bool insertFunction(BPatch_binaryEdit *appBin, BPatch_image *appImage, 
	std::vector<BPatch_function*> insertFuncs, std::vector<BPatch_snippet*> insertFuncArgs,
	const char * instrumentFuncName, BPatch_procedureLocation instrumentPoint);
bool insertMericInitAndClose(BPatch_binaryEdit *appBin, BPatch_image *appImage);
void insertMericInstrumentation(BPatch_binaryEdit *appBin, BPatch_image *appImage);
void loadSharedLibs(BPatch_binaryEdit *appBin, BPatch_binaryEdit *MPIBin = NULL);

void processFile(std::string fileName, std::vector<std::string> * elements);
void splitString(std::string s, std::vector<std::string> * param);
void handleParameters(int argc, char *argv[]);
void printHelp();
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
};
Tparametres params = {"", "instrumentedBin", "", "", "", "", false};

////////////////////////////////////////////////////////////////////////////////


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
	appImage->findFunction("MERIC_Init", initFuncs, true, true, true);
	appImage->findFunction("MERIC_Close", closeFuncs, true, true, true);
	if (initFuncs.size() == 0 || closeFuncs.size() == 0)
	{
		std::cerr<< "Could not find MERIC_Init/Close function\n";
		return false;
	}

	if(params.mpi)
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
		std::cerr<< "Could not find MERIC_MeasureStart function\n";
		return;
	}
	if (stopFuncs.size() == 0)
	{
		std::cerr<< "Could not find MERIC_MeasureStop function\n";
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
		if (insertFunction(appBin, appImage, startFuncs, startArgs, funcName.c_str(), BPatch_entry))
			//TODO end execution in case of function call insertion fail
			insertFunction(appBin, appImage, stopFuncs,  stopArgs,  funcName.c_str(), BPatch_exit);
	}
}


/**
 * load necessary shared libraries to the binary
 * if we instrument (Init and Close) libmpi.so we must add them all too
 **/
void loadSharedLibs(BPatch_binaryEdit *appBin, BPatch_binaryEdit *MPIBin = NULL)
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

////////////////////////////////////////////////////////////////////////////////

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
 * split a string according delimiter '=' a returns them in vector
 **/
void splitString(std::string s, std::vector<std::string> * param)
{
	const std::string delimiter = "=";
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos)
	{
		(*param).push_back(s.substr(0, pos));
		s.erase(0, pos + delimiter.length());
	}
	(*param).push_back(s);
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
		splitString(std::string(argv[i]), &flag);

		if (flag[0] == "--input")
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
		}else
		{
			printHelp();
		}
	}

	if (params.input == "" || params.funclist == "") //compulsory
		printHelp();
}

//TODO mangled names
void printHelp()
{
	std::cerr << "This Dyninst application instruments application functions in the provided binary with MERIC functions\n"
		<< "COMPULSORY PARAMETERS:\n"
		<< "\t --input=binary \n"
		<< "\t      = input binary name\n"
		<< "\t --funclist=file\n"
		<< "\t      = input file with functions to instrument, function per line\n"
		"\n"
		"OPTIONAL PARAMETERS:\n"
		<< "\t --output=binary\n"
		<< "\t      = name of the output binary\n"
		<< "\t --mpi=[path/to/libmpi.so]\n"
		<< "\t      = parameter must be used for mpi applications\n"
		<< "\t      if instrumented version of libmpi.so already exists, \n"
		<< "\t      than the path to libmpi.so does not have to be specified\n"
		<< "\t --dependencies=file\n"
		<< "\t      = list of shared libraries that should be added, \n"
		<< "\t      if not specified full list of libraries is applied\n"
		<< "\t --omp=libomp.so\n"
		<< "\t      = if the instrumented binary does not use openmp, however MERIC with\n"
		<< "\t      OpenMP support is demand, than the libomp.so (libgomp.so; libiomp5.so; ...)\n"
		<< "\t      dependency can by added using this parameter\n\n";
	exit(1);
}

////////////////////////////////////////////////////////////////////////////////
int main (int argc, char *argv[])
{
	handleParameters(argc, argv);

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
	
	processFile(params.funclist, &funcFilterList);
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
	return EXIT_FAILURE;
}





