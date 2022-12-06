#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <mpi.h>

#include <meric.h>
#include "work.h"

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);
	MERIC_Init();

	int rank, size;
	double time;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	MERIC_MeasureStart("A");

	if (rank%2)
	{
		do_work(10);
	}
	else
		sleep(10);
	

	MERIC_MeasureStop("A");

	MPI_Barrier(MPI_COMM_WORLD); //not necessary, just for the test purpose
	
	MERIC_Close();
	MPI_Finalize();
	return 0;
}



