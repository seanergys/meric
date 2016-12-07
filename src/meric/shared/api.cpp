
#include <cstdlib>

#include "../../../include/meric.h"
#include "meric.h"

#include "../../wrapper/environmentwrapper.h"
#include "../../wrapper/perfeventwrapper.h"
#include "../../wrapper/papiwrapper.h"
#include "../../wrapper/raplwrapper.h"

meric::SharedMeric *holder = NULL;

void MERIC_Init()
{
    std::cout << "INIT" << std::endl;

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
	if (meric::Meric::MODE == meric::MODE_RAPL || meric::MODE_BOTH)
		meric::RAPL::init();
}

void MERIC_Close()
{
    std::cout << "CLOSE" << std::endl;

	if (holder == NULL) {
		std::cerr << "MERIC ERROR: call MERIC_Close() before MERIC_Init()\n";
		exit(EXIT_FAILURE);
	}

	if (meric::Meric::COUNTERS == "perfevent")
		meric::PERFEVENT::close();
	if (meric::Meric::COUNTERS == "papi")
		meric::PAPI::close();
// close() is not needed for RAPL
//	if (meric::Meric::MODE != meric::MODE_HDEEM)
//		meric::RAPL::close();
	holder->save();


	delete holder;
	holder = NULL;
    std::cout << "MERIC FIN" << std::endl;
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




