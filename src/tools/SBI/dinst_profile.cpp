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
enum MODETYPE {DEMENGLED, MENGLED, TYPED};
MODETYPE mode = DEMENGLED;
bool printAllLibs = false;

void printHelp()
{
	std::cerr << "This Dyninst application list all the functions in the binary\n"
	          << "The functions are grouped according their module\n"
	          << "Instrumentable functions are marked with \"+\" sign, otherwise with \"-\"\n"
	          << "\n"
	          << "./dins_profile [nmtl] binary\n"
	          << "-d\t print demangled function name [default] \n"
	          << "-m\t print mangled function name\n"
	          << "-t\t print typed function names\n"
	          << "-l\t print functions from shared libraries\n"
	          << "\n";
	exit (1);
}

void handleParameters(int argc, char *argv[])
{
	if (argc == 1)
	{
		printHelp();
	}

	for (int i = 1; i < argc-1; ++i)
	{
		std::string arg = argv[i];
		for(char& flag : arg.substr(1))
		{
			if (flag == 'd')
			{
				mode = DEMENGLED;
			}else
			if (flag == 'm')
			{
				mode = MENGLED;
			}else if (flag == 't')
			{
				mode = TYPED;
			}else if (flag == 'l')
			{
				printAllLibs = true;
			}else
			{
				printHelp();
			}
		}
	}
}

int main (int argc, char *argv[])
{

	handleParameters(argc, argv);

	BPatch_binaryEdit *appBin = bpatch.openBinary (argv[argc-1], printAllLibs);
	if (appBin == NULL)
	{
		printHelp();
		std::cerr << "Failed to open binary: " << argv[argc-1] << std::endl;
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
			if (mode == DEMENGLED)
				std::cout << "\t" << (curFunc->isInstrumentable() ? "+ " : "- ") << curFunc->getDemangledName() << std::endl;
			else if (mode == MENGLED)
				std::cout << "\t" << (curFunc->isInstrumentable() ? "+ " : "- ") << curFunc->getMangledName() << std::endl;
			else //TYPED
				std::cout << "\t" << (curFunc->isInstrumentable() ? "+ " : "- ") << curFunc->getTypedName() << std::endl;
		}
	}
	return 0;
}

















