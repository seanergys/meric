
#include "mpi.h"
#include <cstdlib>

#include "../../../include/meric.h"
#include "meric.h"

meric::DistributedMeric *holder = NULL;

void MERIC_Init()
{
	int initialized, finalized;
	MPI_Initialized(&initialized);
	MPI_Finalized(&finalized);
	if (!initialized) {
		std::cerr << "MERIC ERROR: MERIC_Init() called before MPI_Init()\n";
		exit(EXIT_FAILURE);
	}
	if (finalized) {
		std::cerr << "MERIC ERROR: MERIC_Init() called after MPI_Finalize()\n";
		exit(EXIT_FAILURE);
	}
	if (holder != NULL) {
		std::cerr << "MERIC ERROR: MERIC_Init() called before MERIC_Close()\n";
		exit(EXIT_FAILURE);
	}

	meric::Meric::readRegionParameters();
	holder = new meric::DistributedMeric();
}

void MERIC_Close()
{
	MERIC_INFO << "CLOSEing\n";
	int finalized;
	MPI_Finalized(&finalized);
	if (finalized) {
		std::cerr << "MERIC ERROR: MERIC_Close() called after MPI_Finalize()\n";
		exit(EXIT_FAILURE);
	}
	if (holder == NULL) {
		std::cerr << "MERIC ERROR: MERIC_Close() called before MERIC_Init()\n";
		exit(EXIT_FAILURE);
	}
	MERIC_INFO << "SAVEing\n";
	holder->save();
	MERIC_INFO << "SAVEd\n";

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



