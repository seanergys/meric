
#include "mpi.h"
#include <cstdlib>

#include "../../../include/meric.h"
#include "meric.h"

#include "../../wrapper/environmentwrapper.h"
#include "../../wrapper/perfeventwrapper.h"
#include "../../wrapper/papiwrapper.h"
#include "../../wrapper/raplwrapper.h"

meric::DistributedMeric *holder = NULL;

void MERIC_Init()
{
	int initialized, finalized;
	MPI_Initialized(&initialized);
	MPI_Finalized(&finalized);
	if (!initialized) {
		std::cerr << "MERIC ERROR: call MERIC_Init() before MPI_Init()\n";
		exit(EXIT_FAILURE);
	}
	if (finalized) {
		std::cerr << "MERIC ERROR: call MERIC_Init() after MPI_Finalize()\n";
		exit(EXIT_FAILURE);
	}
	if (holder != NULL) {
		std::cerr << "MERIC ERROR: call MERIC_Init() before MERIC_Close()\n";
		exit(EXIT_FAILURE);
	}

	meric::Meric::readRegionParameters();
	holder = new meric::DistributedMeric();

	//wrappers must be initialized after meric instantiation
	//rapl counter expects that environment::init() was already called
	if (meric::Meric::COUNTERS == "perfevent")
		meric::PERFEVENT::init();
	if (meric::Meric::COUNTERS == "papi")
		meric::PAPI::init();

	if (meric::Meric::MODE == meric::MODE_RAPL || meric::Meric::MODE == meric::MODE_BOTH)
		meric::RAPL::init();
}

void MERIC_Close()
{
	int finalized;
	MPI_Finalized(&finalized);
	if (finalized) {
		std::cerr << "MERIC ERROR: call MERIC_Close() after MPI_Finalize()\n";
		exit(EXIT_FAILURE);
	}
	if (holder == NULL) {
		std::cerr << "MERIC ERROR: call MERIC_Close() before MERIC_Init()\n";
		exit(EXIT_FAILURE);
	}

	if (meric::Meric::COUNTERS == "perfevent")
		meric::PERFEVENT::close();
	if (meric::Meric::COUNTERS == "papi")
		meric::PAPI::close();
// close() is not needed for RAPL
//	if (meric::Meric::MODE == meric::MODE_RAPL || meric::MODE_BOTH)
//		meric::RAPL::close();

	holder->save();

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



