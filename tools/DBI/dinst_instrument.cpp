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


char *inBinary = NULL;
BPatch bpatch;
std::set<std::string> funcFilterList;
const std::string FILTERFILE = "DINST_FILTER";

////////////////////////////////////////////////////////////////////////////////

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
 * Insert MERIC_Init() and MERIC_Close() 
 * The functions should be the first and the last function calls in the application
 * In case of MPI application Init follows MPI_Init and Close precedes MPI_Finalize
 **/
// TODO MPI
bool insertMericInitAndClose(BPatch_binaryEdit *appBin, BPatch_image *appImage)
{

	std::vector<BPatch_function*> functions;
	appImage->findFunction("main", functions);

	//Insert MERIC_Init	
	std::vector<BPatch_point *> *points;
	points = functions[0]->findPoint(BPatch_entry);
	if (!points || points->size() == 0)
	{
		std::cerr << "No entry points for " << "main" << std::endl;
		return false;
	}

	std::vector<BPatch_snippet*> mericArgs;

	std::vector<BPatch_function*> initFuncs;
	appImage->findFunction("MERIC_Init", initFuncs);

	if (initFuncs.size() == 0)
	{
		std::cerr<< "Could not find printf\n";
		return false;
	}
	else
	{
		BPatch_funcCallExpr initCall(*(initFuncs[0]), mericArgs);
		if (!appBin->insertSnippet(initCall, *points))
		{
			std::cerr << "insertSnippet "<< "main" <<" failed\n";
			return false;
		}
	}


	//Insert MERIC_Close
	points = functions[0]->findPoint(BPatch_exit);
	if (!points || points->size() == 0)
	{
		std::cerr << "No exit points for " << "main" << std::endl;
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
			std::cerr << "insertSnippet "<< "main" <<" failed\n";
			return false;
		}
	}
	return true;
}


/**
 * Instrumentation of the selected function
 **/
void insertMericInstrumentation(BPatch_binaryEdit *appBin, BPatch_image *appImage)
{
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
	for (std::string funcName : funcFilterList)
	{
		std::cout << funcName << std::endl;

		std::vector<BPatch_function*> functions;
		appImage->findFunction(funcName.c_str(), functions);

// BPatch_entry, funcName, appBin, startFuncs[0]

		//Insert MERIC_MeasureStart(funcName)
		std::vector<BPatch_point *> *entryPoints;
		entryPoints = functions[0]->findPoint(BPatch_entry);
		if (!entryPoints || entryPoints->size() == 0)
		{
			std::cerr << "No entry points for " << funcName << std::endl;
		}
		
		std::vector<BPatch_snippet*> startArgs;
		BPatch_snippet* param = new BPatch_constExpr(funcName.c_str());
		startArgs.push_back(param);

		BPatch_funcCallExpr startCall(*(startFuncs[0]), startArgs);
		if (!appBin->insertSnippet(startCall, *entryPoints))
		{
			std::cerr << "insertSnippet "<< funcName <<" failed\n";
		}

		//Insert MERIC_MeasureStop()
		std::vector<BPatch_point *> *exitPoints;
		exitPoints = functions[0]->findPoint(BPatch_exit);
		if (!exitPoints || exitPoints->size() == 0)
		{
			std::cerr << "No exit points for " << funcName << std::endl;
		}
		
		std::vector<BPatch_snippet*> stopArgs;

		BPatch_funcCallExpr stopCall(*(stopFuncs[0]), stopArgs);
		if (!appBin->insertSnippet(stopCall, *exitPoints))
		{
			std::cerr << "insertSnippet "<< funcName <<" failed\n";
		}
	}
}


int main (int argc, char *argv[])
{
	//TODO read name of the input and output binary from command line
	inBinary = "test";	//input binary name

	/* Open the specified binary for binary rewriting. 
	* When the second parameter is set to true, all the library dependencies 
	* as well as the binary are opened */

	BPatch_binaryEdit *appBin = bpatch.openBinary (inBinary, false);
	if (appBin == NULL) {
		std::cerr << "Failed to open binary" << std::endl;
		return EXIT_FAILURE;
	}
	
	//TODO get dynamically a list of necessary shares libraries that should be added to the binary
    if (!appBin->loadLibrary("libmeric.so") || !appBin->loadLibrary("libnuma.so"))
    {
    	std::cerr << "Library load failed\n";
    	return 0;
    }
	
	BPatch_image *appImage = appBin->getImage();

	if(insertMericInitAndClose(appBin, appImage))
	{
		processFilterFile();
	
		insertMericInstrumentation(appBin, appImage);
	
		// Output the instrumented binary
		std::cerr << "NEW binary writing\n";
		if (!appBin->writeFile ("newTest"))
		{
			std::cerr << "Failed to write output binary\n";
			return EXIT_FAILURE;
		}
	}
	std::cerr << "FIN\n";

	return 0;
}





















