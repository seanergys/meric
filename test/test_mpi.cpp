
#include <meric.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include "mpi.h"

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);
	MERIC_Init();

	int rank, size;
	double time;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	MERIC_MeasureStart("A");

		for(int i = 0; i < 1; i++)
		{
			MERIC_MeasureStart("iteration");

				MERIC_MeasureStart("B");
					sleep(1);
				time = MERIC_MeasureStop(); //B
				if (time > 0.0)
					std::cout << "["<<rank<<"] B: "<< time << std::endl;

			    MERIC_MeasureStart("C");
				MERIC_MeasureStart("D");
					sleep(2);
				time = MERIC_MeasureStop(); //D
				if (time > 0.0)
					std::cout << "["<<rank<<"] D: "<< time << std::endl;

			    time = MERIC_MeasureStop(); //C
				if (time > 0.0)
					std::cout << "["<<rank<<"] C: "<< time << std::endl;
					
	    		MERIC_MeasureStart("B");
		    		sleep(1);
		    	time = MERIC_MeasureStop(); //B
				if (time > 0.0)
					std::cout << "["<<rank<<"] B: "<< time << std::endl;

		    MERIC_MeasureStop(); //iteration
		}

		sleep(1);

	MERIC_MeasureStop(); //A

	MPI_Barrier(MPI_COMM_WORLD); //not necessary, just for the test purpose 
	
	MERIC_Close();
	MPI_Finalize();
	return 0;
}



