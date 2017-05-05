
#include <cstdlib>

#include "../../../include/meric.h"
#include "meric.h"

#include "../../wrapper/environmentwrapper.h"
#include "../../wrapper/perfeventwrapper.h"
#include "../../wrapper/papiwrapper.h"
#include "../../wrapper/raplwrapper.h"
#include "../../wrapper/jetsonwrapper.h"

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

	//wrappers must be initialized after meric instantiation
	//rapl counter expects that environment::init() was already called
	if (meric::Meric::COUNTERS == "perfevent")
		meric::PERFEVENT::init();
	if (meric::Meric::COUNTERS == "papi")
		meric::PAPI::init();
	if (meric::Meric::MODE == meric::MODE_RAPL || meric::Meric::MODE == meric::MODE_BOTH)
	{
		if (meric::Meric::DETAILED)
			meric::RAPL::detailed();
		meric::RAPL::init();
	}
	if (meric::Meric::MODE == meric::MODE_JETSON)
		meric::JETSON::init();
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




