/**
 * This Dyninst application provides an application SBI with TIMEPROF and MERIC
 *
 **/

#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <algorithm>

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
#include "Instruction.h" //required for countim number of instructions per function


////////////////////////////////////////////////////////////////////////////////
BPatch bpatch;

struct Tparametres{
	std::string input;
	std::string output;
	std::string funclist;
	std::string dependencies;
	std::string mpilib;
	std::string language;
	bool mpi;
	bool omplib;
	bool timeprof;
	bool meric;
	uint instlimit;
	std::vector <const char *> argv;
};
Tparametres params = {"", "instrumentedBin", "", "", "", "", false, true, false, false, 128, {}};

struct TfuncName{
	std::string demangled;
	std::string mangled;
};

std::vector<TfuncName> funcFilterList;

////////////////////////////////////////////////////////////////////////////////
void printHelp();
void handleParameters(int argc, char *argv[]);
void processFile(std::string fileName, std::vector<std::string> * elements);
void splitString(std::string s, const char delimiter, std::vector<std::string> * param);
template<typename TBpatch> bool insertFunction(TBpatch *appBin, BPatch_image *appImage, 
	std::vector<BPatch_function*> insertFuncs, std::vector<BPatch_snippet*> insertFuncArgs,
	const char * instrumentFuncName, BPatch_procedureLocation instrumentPoint);
void readFilterFile(BPatch_image *appImage);
int writeBinary(BPatch_binaryEdit *binary, const char * binaryName);
bool insertInitAndClose(BPatch_binaryEdit *appBin, BPatch_image *appImage);
void insertRegionStartStopInstrumentation(BPatch_binaryEdit *appBin, BPatch_image *appImage);
void loadSharedLibs(BPatch_binaryEdit *appBin, BPatch_binaryEdit *MPIBin = NULL);
int main (int argc, char *argv[]);

// AUXILIARY FUNCTIONS /////////////////////////////////////////////////////////
void printHelp()
{
	std::cerr << "\nThis Dyninst application provides an application static binary instrumentation with TIMEPROF and MERIC\n"
		<< "\n"
		<< "APPLICATION USE SELECTION:\n"
		<< " Only one of following parameters must be specified\n"
		<< "\t --meric\n"
		<< "\t      = instrumentation of the application with MERIC\n"
		<< "\t --timeprof\n"
		<< "\t      = instrumentation of the application with TIMEPROF\n"
		<< "\n"
		<< "COMPULSORY PARAMETERS:\n"
		<< "\t --input=binary \n"
		<< "\t      = input binary name\n"
		<< "\t --funclist=file\n"
		<< "\t      = input file with functions to instrument, function name per line\n"
		<< "\t      for TIMEPROF file can be empty - will instrument all available functons\n"
		<< "\n"
		<< "OPTIONAL PARAMETERS:\n"
		<< "\t --output=binary\n"
		<< "\t      = name of the output binary\n"
		<< "\t      if not specified the default name 'instrumentedBin' is used\n"
		<< "\t --mpi=[path/to/libmpi.so]\n"
		<< "\t      = parameter must be used for MPI applications\n"
		<< "\t      if instrumented version of libmpi.so already exists, \n"
		<< "\t      than the path to libmpi.so does not have to be specified\n"
		<< "\t --noopenmp\n"
		<< "\t      = if the instrumented use only MPI but not OpenMP\n"
		<< "\t      binary does not use OpenMP, however MERIC with\n"
		<< "\t --language=[c/c++/fortran]\n"
		<< "\t      = please, specify if Dyninst cannot resolve language type of the binary\n"
		<< "\n"
		<< "OPTIONAL PARAMETERS - MERIC ONLY:\n"
		<< "\t --dependencies=file\n"
		<< "\t      = list of shared libraries that should be added, \n"
		<< "\t      if not specified full list of libraries is applied\n"
		<< "\n"
		<< "OPTIONAL PARAMETERS - TIMEPROF ONLY:\n"
		<< "\t --instlimit=value\n"
		<< "\t      = minimum number of instructions per function to function be unstrumented\n"
		<< "\t      if not specified default value 128 inctructions per functions is used\n"
		<< "\n";
	exit(1);
}

/**
 * reads command line parametrs and sets global variable 'parameters'
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
		
		if (flag[0] == "--timeprof")
		{
			params.timeprof = true;
		}
		else if (flag[0] == "--meric")
		{
			params.meric = true;
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
		else if (flag[0] == "--noopenmp")
		{
			params.omplib = false;
		}
		else if (flag[0] == "--language")
		{
			if(flag.size() != 2)
				printHelp();
			params.language = flag[1];			
		}
		else if (flag[0] == "--instlimit")
		{
			if(flag.size() != 2)
				printHelp();
			try
			{
				params.instlimit = std::stoi(flag[1]);
			}
			catch (...)
			{
				printHelp();
			}
		}else
		{
			printHelp();
		}
	}

	if (params.input == "" || params.funclist == "" || (params.timeprof == params.meric)) //compulsory
		printHelp();
	
	if (!params.omplib && !params.mpi)
	{
		std::cerr << "ERROR Non-parallelized applications not supported!\n";
		exit(2);
	}
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
		if(inputfile.is_open())
		{
			std::string element;
			while (inputfile >> element)
				(*elements).push_back(element);
			inputfile.close();
		}
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

// DYNINST FUNCTIONALITY ///////////////////////////////////////////////////////

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
		if (params.meric)
		{
			if (params.mpi)
			{
				if (params.omplib)
					extraSharedLibraries.push_back("libmericmpi.so");
				else
					extraSharedLibraries.push_back("libmericmpionly.so");
			}
			else //OpenMP only
			{
				extraSharedLibraries.push_back("libmeric.so");
			}
			extraSharedLibraries.push_back("libx86_adapt.so");
			extraSharedLibraries.push_back("libmsr.so");
			extraSharedLibraries.push_back("libpapi.so");
			extraSharedLibraries.push_back("libnuma.so");
			extraSharedLibraries.push_back("libhdeem.so");
			extraSharedLibraries.push_back("libfreeipmi.so");
			extraSharedLibraries.push_back("libcpufreq.so");
			extraSharedLibraries.push_back("libcpupower.so");
			extraSharedLibraries.push_back("libnvidia-ml.so");
			extraSharedLibraries.push_back("libe_smi64.so");
//			extraSharedLibraries.push_back("libpthread.so");	//so-far always N/A
		}
		else //params.timeprof
		{
			if (params.mpi)
			{
				if (params.omplib)
					extraSharedLibraries.push_back("libtimeprofmpi.so");
				else
					extraSharedLibraries.push_back("libtimeprofmpionly.so");
			}
			else //OpenMP only
			{
				extraSharedLibraries.push_back("libtimeprof.so");
			}
		}
		extraSharedLibraries.push_back("librt.so");
	}

	std::cerr << "<> Inserting shared libraries dependencies - APP\n";
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
		std::cerr << "<> No "<< instrumentFuncName <<" to instrument\n";
		return false;
	}
	
	points = functions[0]->findPoint(instrumentPoint);
	if (!points || points->size() == 0)
	{
		std::cerr << "<> No points for " << instrumentFuncName << std::endl;
		return false;
	}

	BPatch_funcCallExpr insertCall(*(insertFuncs[0]), insertFuncArgs);
	if (!appBin->insertSnippet(insertCall, *points))
	{
		std::cerr << "<> insertSnippet for "<< instrumentFuncName <<" failed\n";
		return false;
	}
	return true;
}

/**
 * reads a file with names of functions to instrument
 * if the file does not exists or is empty than we instrument all available functions
 **/
void readFilterFile(BPatch_image *appImage)
{
	funcFilterList.clear();
	std::vector <std::string> funcNames;
	processFile(params.funclist, &funcNames);
	if (funcNames.empty() && params.timeprof)
	{
		std::vector < BPatch_module * >*modules = appImage->getModules ();
		std::vector < BPatch_module * >::iterator moduleIter;
		for (moduleIter = modules->begin (); moduleIter != modules->end (); ++moduleIter)
		{
			if ((*moduleIter)->isSharedLib ())
				continue;

//			char moduleName[1024];
//			(*moduleIter)->getName (moduleName, 1024);
//			std::cout << "<> Library: " << moduleName << std::endl;

			std::vector < BPatch_function * >*allFunctions = (*moduleIter)->getProcedures();
			std::vector < BPatch_function * >::iterator funcIter;
			for (funcIter = allFunctions->begin (); funcIter != allFunctions->end (); ++funcIter)
			{
//--------------//to eliminate small functions we evaluate number of instructions per function
				BPatch_flowGraph* fg = (*funcIter)->getCFG();
				std::set<BPatch_basicBlock*> blocks;
				fg->getAllBasicBlocks(blocks);
				size_t bb_cnt = blocks.size();
				size_t inst_cnt = 0;

				for (auto block_iter = blocks.begin(); block_iter != blocks.end(); ++block_iter)
				{
					BPatch_basicBlock* block = *block_iter;
#if DYNINST_MAJOR_VERSION < 10
					std::vector<Dyninst::InstructionAPI::Instruction::Ptr> insns;
#else
					std::vector<Dyninst::InstructionAPI::Instruction> insns;
#endif
					block->getInstructions(insns);
					inst_cnt += insns.size();
					//it is also possible to determine a type of the instruction
				}
//--------------//another heurictic how to detect small functions
				Dyninst::Address addrStart;
				Dyninst::Address addrStop;
				(*funcIter)->getAddressRange(addrStart, addrStop);

//----------------------------------------------------------------------------//
				std::string funcName = (*funcIter)->getDemangledName();
//				std::string funcName = (*funcIter)->getMangledName();
				

//				if (inst_cnt > 37)	//ESPRESO
//					continue;

				if (inst_cnt > params.instlimit && //some small funcs may cause problems to Dyninst
					funcName.substr (0,5) != "std::" && 
					funcName.substr (0,5) != "MPI::" && 
					funcName.substr(0,9) != "thunk to " &&
					funcName.substr(0,15) != "_GLOBAL__sub_I_" &&
					funcName != "__static_initialization_and_destruction_0"

					&& funcName.substr(0,8) != "__intel_"
					&& funcName.substr(0,7) != "_intel_"
					)
				{
					std::cout << (*funcIter)->getTypedName() << std::endl;
//					std::cout << "A: "<<addrStop - addrStart 
//					          << "\tBB: "<<bb_cnt
//					          << "\tIS: "<<inst_cnt
//					          << "\t" <<funcName << std::endl;
					TfuncName name = {funcName, (*funcIter)->getMangledName()};
					funcFilterList.push_back(name);
				}
			}
		}
	}
	else
	{
		for (auto fnc : funcNames)
		{
			//both mangled and demangled names set despite it is not true	//TODO - will depend on final profiler solution
			TfuncName name = { fnc, fnc };
			funcFilterList.push_back(name);
		}
	}
}

/**
 * Writes new edited version of the binary
 **/
int writeBinary(BPatch_binaryEdit *binary, const char * binaryName)
{
	std::cerr << "<> NEW binary writing - "<< binaryName << std::endl;
	if (!binary->writeFile (binaryName))
	{
		std::cerr << "<> Failed to write output binary\n";
		return 1;
	}
	return 0;
}

/**
 * Inserts initialisation and closing functions
 * these functions rely on main of MPI_Init/Finalize functions
 **/
bool insertInitAndClose(BPatch_binaryEdit *appBin, BPatch_image *appImage)
{
	std::vector<BPatch_function*> functions;
	std::vector<BPatch_snippet*>  funcArgs;
	std::vector<BPatch_function*> initFuncs;
	std::vector<BPatch_function*> closeFuncs;
	
	if(params.meric)
	{
		appImage->findFunction("MERIC_Init", initFuncs, true, true, true);
		appImage->findFunction("MERIC_Close", closeFuncs, true, true, true);
		
		if (initFuncs.empty() || closeFuncs.empty())
		{
			std::cerr<< "<> Could not find MERIC_Init/Close function\n";
			return false;
		}
	}
	else //params.timeprof
	{
		appImage->findFunction("TIMEPROF_evaluate", closeFuncs, true, true, true);

		if (closeFuncs.empty())
		{
			std::cerr<< "<> Could not find TIMEPROF_evaluate function\n";
			return false;
		}
	}

	int ret = true;
	if(params.mpi)
	{
		if (!initFuncs.empty())
			ret = ret && insertFunction<BPatch_binaryEdit>(appBin, appImage, initFuncs, funcArgs, "MPI_Init", BPatch_exit);
		if (!closeFuncs.empty())
			ret = ret && insertFunction<BPatch_binaryEdit>(appBin, appImage, closeFuncs, funcArgs, "MPI_Finalize", BPatch_entry);
	}
	else
	{
		std::vector<BPatch_sourceObj*> sourceObjs;
		appImage->getSourceObj(sourceObjs);
		BPatch_language language = sourceObjs[0]->getLanguage();
/*
typedef enum BPatch_language {
		BPatch_c,
		BPatch_cPlusPlus,
		BPatch_fortran,
		BPatch_fortran77,
		BPatch_fortran90,
		BPatch_f90_demangled_stabstr,
		BPatch_fortran95,
	BPatch_assembly,
	BPatch_mixed,
	BPatch_hpf,
	BPatch_java,
	BPatch_unknownLanguage
} BPatch_language;		
*/
		std::string mainFunction;
		if (language == BPatch_c ||
		    language == BPatch_cPlusPlus ||
		    params.language == "c" ||
		    params.language == "c++") //"main"
		{
			//std::cerr << "<> Instrumenting C/C++ application\n";
			mainFunction = "main";
		}
		else if (language == BPatch_fortran ||
		    language == BPatch_fortran77 || 
		    language == BPatch_fortran90 || 
		    language == BPatch_f90_demangled_stabstr || 
		    language == BPatch_fortran95 ||
		    params.language == "fortran") //"MAIN__"
		{
			//std::cerr << "<> Instrumenting Fortran application\n";
			mainFunction = "MAIN__";
		}
		else
		{
			std::cerr << "<> ERROR: Unknown or unsupported language type! Please, specify '--language' parameter.\n";
			return false;
		}
		if (!initFuncs.empty())
			ret = ret && insertFunction<BPatch_binaryEdit>(appBin, appImage, initFuncs, funcArgs, mainFunction.c_str(), BPatch_entry);
		if (!closeFuncs.empty())
			ret = ret && insertFunction<BPatch_binaryEdit>(appBin, appImage, closeFuncs, funcArgs, mainFunction.c_str(), BPatch_exit);
	}
	if (!ret)
	{
		if(params.meric)
			std::cerr<< "<> Could not insert MERIC_Init/Close function\n";
		else
			std::cerr<< "<> Could not insert TIMEPROF_evaluate function\n";
		return false;
	}
	
	return ret;
}

/**
 * Instrumentation of the selected functions
 * function wrapping with measurement start-stop
 **/
void insertRegionStartStopInstrumentation(BPatch_binaryEdit *appBin, BPatch_image *appImage)
{
	std::cerr << "<> Inserting instrumentation of application functions ("<< funcFilterList.size()<<")\n";

	std::vector<BPatch_function*> startFuncs; 
	std::vector<BPatch_function*> stopFuncs;
	const char * regStartFunc;
	const char * regStopFunc;
	if(params.meric)
	{
		regStartFunc = "MERIC_MeasureStart";
		regStopFunc  = "MERIC_MeasureStop";
	}
	else
	{
		regStartFunc = "TIMEPROF_regionStart";
		regStopFunc  = "TIMEPROF_regionStop";
	}

	appImage->findFunction(regStartFunc, startFuncs, true, true, true);
	appImage->findFunction(regStopFunc, stopFuncs, true, true, true);
	if (startFuncs.size() == 0)
	{
		std::cerr << "<> Could not find "<<regStartFunc<<" function\n";
		return;
	}
	if (stopFuncs.size() == 0)
	{
		std::cerr << "<> Could not find "<<regStopFunc<<" function\n";
		return;
	}
	
	//Instrument functions from the list
	for (auto funcName : funcFilterList)
	{
//		std::cout << "\t" << funcName.demangled << std::endl;

		std::vector<BPatch_snippet*> startArgs;
		std::vector<BPatch_snippet*> stopArgs;
		//BPatch_snippet* param = new BPatch_constExpr(funcName.demangled.c_str());
		BPatch_snippet* param = new BPatch_constExpr(funcName.mangled.c_str());
		startArgs.push_back(param);
		if (params.timeprof)
			stopArgs.push_back(param);
		if (insertFunction<BPatch_binaryEdit>(appBin, appImage, startFuncs, startArgs, funcName.mangled.c_str(), BPatch_entry))
			//TODO end execution in case of function call insertion fail
			insertFunction<BPatch_binaryEdit>(appBin, appImage, stopFuncs,  stopArgs,  funcName.mangled.c_str(), BPatch_exit);
	}
}


////////////////////////////////////////////////////////////////////////////////

int instrumentApplication()
{
	// open the binary + extra shared libs
	BPatch_binaryEdit *appBin = bpatch.openBinary (params.input.c_str(), false);
	if (appBin == NULL)
	{
		std::cerr << "<> Failed to open binary" << std::endl;
		return EXIT_FAILURE;
	}
	
	BPatch_image *MPIImage;
	BPatch_binaryEdit *MPIBin;
	if (params.mpilib != "")
	{
		MPIBin = bpatch.openBinary (params.mpilib.c_str(), false);
		if (MPIBin == NULL)
		{
			std::cerr << "<> Failed to open MPI binary" << std::endl;
			return EXIT_FAILURE;
		}
		MPIImage = MPIBin->getImage();
		loadSharedLibs(appBin, MPIBin);
	}
	else
		loadSharedLibs(appBin);

	// get list of functions to instrument
	BPatch_image *appImage = appBin->getImage();
	readFilterFile(appImage);
	if (funcFilterList.empty())
	{
		std::cerr << "<> List of functions to instrument is empty\n";
		return 1;
	}

	// instrumentation of the binary
	if (params.mpi)
	{
		if (params.mpilib != "")
		{
			if (insertInitAndClose(MPIBin, MPIImage))
			{
				if (writeBinary(MPIBin, "libmpi.so"))
					return EXIT_FAILURE;
			}
		}
	}
	else
	{
		if (!insertInitAndClose(appBin, appImage))
		{
			std::cerr << "FAIL\n";
			return EXIT_FAILURE;
		}
	}
	
	insertRegionStartStopInstrumentation(appBin, appImage);

	// write the instrumented binary
	if (!writeBinary(appBin, params.output.c_str()))
	{
		std::cerr << "<> Static Binary Instrumentation successfully finished\n";
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
	return instrumentApplication();
}





