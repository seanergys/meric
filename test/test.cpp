#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include <meric.h>
#include "work.h"

//RAII supported by Clang and GNU compilers, so the following region will not be available if using another compiler
void raiiTest(int time)
{
	MERIC_CaptureScope("RAII");
	do_work(time);
}

int main(int argc, char **argv)
{
	MERIC_Init();
	
	for(int i = 0; i < 1; i++)
	{
		#pragma omp parallel
		{
			MERIC_MeasureStart("SLEEP");
			usleep(500000);

				MERIC_MeasureStart("Nested");
				usleep(500000);
				MERIC_MeasureStop("Nested");

			MERIC_MeasureStop("SLEEP");

			MERIC_MeasureStart("RUN");
			do_work(1);
			MERIC_MeasureStop("RUN");
		} //omp parallel
		
		raiiTest(1);
	}
	
	MERIC_Close();
	return 0;
}
