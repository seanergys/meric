#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>

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

std::set < std::string > skipLibraries;
char *inBinary = NULL;
BPatch bpatch;

#define NAME_MAX_SIZE 1024

void printHelp()
{
	std::cerr << "This Dyninst application list all the functions in the binary\n"
	          << "The functions are grouped according their module\n"
	          << "Instrumentable functions are marked with \"+\" sign, othewise with \"-\"\n"
	          << "\n"
	          << "Application takes one argument - name of the binary to analyse\n"
	          << "\n";
}

int main (int argc, char *argv[])
{

	if (argc != 2 || std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")
	{
		printHelp();
		return EXIT_FAILURE;
	}

	BPatch_binaryEdit *appBin = bpatch.openBinary (argv[1], false);
	if (appBin == NULL)
	{
		printHelp();
		std::cerr << "Failed to open binary: " << argv[1] << std::endl;
		return EXIT_FAILURE;
	}
	
	BPatch_image *appImage = appBin->getImage();

	std::vector < BPatch_module * >*modules = appImage->getModules ();
	std::vector < BPatch_module * >::iterator moduleIter;
	for (moduleIter = modules->begin (); moduleIter != modules->end (); ++moduleIter)
	{
		char moduleName[NAME_MAX_SIZE];
		(*moduleIter)->getName (moduleName, NAME_MAX_SIZE);


		if ((*moduleIter)->isSharedLib ())
			std::cout << "Shared library: " << moduleName << std::endl;
		else
			std::cout << "Library: " << moduleName << std::endl;


		std::vector < BPatch_function * >*allFunctions = (*moduleIter)->getProcedures (true);
		std::vector < BPatch_function * >::iterator funcIter;


		for (funcIter = allFunctions->begin (); funcIter != allFunctions->end (); ++funcIter)
		{
			BPatch_function *curFunc = *funcIter;
			//curFunc->getName();
			//curFunc->getMangledName()
			std::cout << "\t" << (curFunc->isInstrumentable() ? "+ " : "- ") << curFunc->getTypedName() << std::endl;
		}
	}
	return 0;
}

















