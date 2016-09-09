
#include <cstdlib>
#include <unistd.h>

#include "../../../include/meric.h"
#include "meric.h"


meric::SharedMeric *holder = NULL;

void MERIC_Init()
{
	if (holder != NULL) {
		std::cerr << "MERIC ERROR: call MERIC_Init() before MERIC_Close()\n";
		exit(EXIT_FAILURE);
	}

	meric::HDEEM::init();
	meric::HDEEM::start();
	holder = new meric::SharedMeric();
}

void MERIC_Close()
{
	if (holder == NULL) {
		std::cerr << "MERIC ERROR: call MERIC_Close() before MERIC_Init()\n";
		exit(EXIT_FAILURE);
	}
	meric::HDEEM::stop();
	holder->save();
	meric::HDEEM::close();

	delete holder;
	holder = NULL;
}

void MERIC_MeasureStart(const char* name)
{
	holder->start(name);
}

void MERIC_MeasureStop()
{
	holder->stop();
}




