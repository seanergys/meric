
#include <cstdlib>

#include "../../../include/meric.h"
#include "meric.h"

meric::SharedMeric *holder = NULL;

void MERIC_Init()
{
    MERIC_INFO << "INIT" << std::endl;

	if (holder != NULL) {
		std::cerr << "MERIC ERROR: call MERIC_Init() before MERIC_Close()\n";
		exit(EXIT_FAILURE);
	}

	// If MERIC_* variables are set/changed at runtime (e.g. from a socket message),
	// refresh cached config before instantiation.
	meric::Meric::refreshConfigFromEnv();

	meric::Meric::readRegionParameters();
	holder = new meric::SharedMeric();
}

void MERIC_Close()
{
    MERIC_INFO << "CLOSE" << std::endl;

	if (holder == NULL) {
		std::cerr << "MERIC ERROR: call MERIC_Close() before MERIC_Init()\n";
		exit(EXIT_FAILURE);
	}

	holder->save();

	delete holder;
	holder = NULL;
    MERIC_INFO << "MERIC FIN" << std::endl;
}

void MERIC_MeasureStart(const char* name)
{
	if (holder == NULL)
	{
		std::cerr << "MERIC WARNING: initialization when region '" << name << "' starts\n";
		MERIC_Init();
	}
	holder->start(name);
}

double MERIC_MeasureStop(const char* name)
{
	return holder->stop(name);
}

double MERIC_MeasureStopStart(const char* stopName, const char* startName)
{
	return holder->stopStart(stopName, startName);
}

void MERIC_IgnoreStart()
{
	holder->startIgnore();
}

void MERIC_IgnoreStop()
{
	holder->stopIgnore();
}

