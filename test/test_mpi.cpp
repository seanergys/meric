
#include <meric.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include "mpi.h"

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);
	MERIC_Init();

	MERIC_MeasureStart("A");

		for(int i = 0; i < 5; i++)
		{
		MERIC_MeasureStart("iteration");

			MERIC_MeasureStart("B");
			        sleep(1);
        		MERIC_MeasureStop(); //B

		        MERIC_MeasureStart("C");
				MERIC_MeasureStart("D");
					sleep(1);
				MERIC_MeasureStop(); //D
		        MERIC_MeasureStop(); //C

        		MERIC_MeasureStart("B");
	        		sleep(1);
	        	MERIC_MeasureStop(); //B

	        MERIC_MeasureStop(); //iteration
		}

		sleep(1);

	MERIC_MeasureStop(); //A

	MERIC_Close();
	MPI_Finalize();
	return 0;
}



