
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
	holder->start(name);
}

void MERIC_MeasureStop()
{
	holder->stop();
}

void MERIC_MeasureStopStart(const char* name)
{
	holder->stopStart(name);
}

void MERIC_IgnoreStart()
{
	holder->startIgnore();
}

void MERIC_IgnoreStop()
{
	holder->stopIgnore();
}

