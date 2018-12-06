
#include <readex.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include "mpi.h"

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);
	READEX_INIT();

	int rank, size;
	double time;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	READEX_REGION_DEFINE(A);
	READEX_REGION_START(A, "A", SCOREP_USER_REGION_TYPE_COMMON);

		READEX_REGION_DEFINE(iteration);
		for(int i = 0; i < 1; i++)
		{
			READEX_REGION_START(iteration, "iteration", SCOREP_USER_REGION_TYPE_COMMON);

				READEX_REGION_START(B, "B", SCOREP_USER_REGION_TYPE_COMMON);
					sleep(1);
				time = READEX_REGION_STOP(B);
				if (time > 0.0)
					std::cout << "["<<rank<<"] B: "<< time << std::endl;

				READEX_REGION_START(C, "C", SCOREP_USER_REGION_TYPE_COMMON);
				READEX_REGION_START(D, "D", SCOREP_USER_REGION_TYPE_COMMON);
					sleep(2);
				time = READEX_REGION_STOP(D);
				if (time > 0.0)
					std::cout << "["<<rank<<"] D: "<< time << std::endl;

			    time = READEX_REGION_STOP(C);
				if (time > 0.0)
					std::cout << "["<<rank<<"] C: "<< time << std::endl;
					
	    		READEX_REGION_START(B, "B", SCOREP_USER_REGION_TYPE_COMMON);
		    		sleep(1);
		    	time = READEX_REGION_STOP(B);
				if (time > 0.0)
					std::cout << "["<<rank<<"] B: "<< time << std::endl;

		    READEX_REGION_STOP(iteration);
		}

		sleep(1);

	READEX_REGION_STOP(A);
	std::cout << "FIN\n";

	MPI_Barrier(MPI_COMM_WORLD); //not necessary, just for the test purpose 
	
	READEX_CLOSE();
	MPI_Finalize();
	return 0;
}



