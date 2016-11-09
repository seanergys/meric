
#include <cstdlib>

#include "../../../include/meric.h"
#include "meric.h"

#include "../../wrapper/environmentwrapper.h"
#include "../../wrapper/perfeventwrapper.h"
#include "../../wrapper/papiwrapper.h"


meric::SharedMeric *holder = NULL;

void MERIC_Init()
{
    std::cout << "INIT" << std::endl;

	if (holder != NULL) {
		std::cerr << "MERIC ERROR: call MERIC_Init() before MERIC_Close()\n";
		exit(EXIT_FAILURE);
	}

	if (meric::COUNTERS == "perfevent")
		meric::PERFEVENT::init();
	if (meric::COUNTERS == "papi")
		meric::PAPI::init();
	meric::Environment::init();

	meric::Meric::readRegionParameters();
	holder = new meric::SharedMeric();
}

void MERIC_Close()
{
    std::cout << "CLOSE" << std::endl;

	if (holder == NULL) {
		std::cerr << "MERIC ERROR: call MERIC_Close() before MERIC_Init()\n";
		exit(EXIT_FAILURE);
	}

	if (meric::COUNTERS == "perfevent")
		meric::PERFEVENT::close();
	if (meric::COUNTERS == "papi")
		meric::PAPI::close();
	meric::Environment::close();

	holder->save();


	delete holder;
	holder = NULL;
}

void MERIC_MeasureStart(const char* name)
{
    std::cout << "START" << std::endl;
	holder->start(name);
}

void MERIC_MeasureStop()
{
    std::cout << "STOP" << std::endl;
	holder->stop();
}




