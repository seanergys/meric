/*
 * export MERIC_CONTINUAL=1 - using HDEEM samples
 * export MERIC_SAMPLES=1 - write samples (id, value) to output files
 * compile meric with -DVERBOSE (edit Makefile)	- extra Meric information to std::cout
 *
 * This test is here to control if the region starts with correct sample number
 * inside the loop are sleep and compute regions - the energy consumption should be much higher at the compute region beginning
 * at the end of compute region the energy doesn't suddenly drop, but it takes a while, however it should be down far earlirer before next compute region will start
 * 
 * in default settings each region takes 10 seconds = 10000 HDEEM blade samples
 */

#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <ctime>
#include <math.h>

#include <meric.h>
#include "work.h"

#define WITHMERIC

int main(int argc, char **argv)
{
	std::cout << "CPU c00ling\n";
	//cpu cooling
	sleep(5);
	std::cout << "Lets's start\n";
	
	//init
	double time1, time2;
	timespec spectime1, spectime2;
	double random = 991611579.454984;
	#ifdef WITHMERIC
	MERIC_Init();
	#endif

	for (int i=0; i<2; i++)
	{
		#ifdef WITHMERIC
		MERIC_MeasureStart("sleep");
			sleep(10);
		MERIC_MeasureStop("sleep");
		#endif

		#ifdef WITHMERIC
		MERIC_MeasureStart("compute");
		#endif
			clock_gettime(CLOCK_REALTIME, &spectime1);
			time1 = timespec2double(spectime1);
			#pragma omp parallel
			do
			{
				random = time1*1095 + log(random);
				clock_gettime(CLOCK_REALTIME, &spectime2);
			}while(timespec2double(spectime2)-time1 < 10.0);
		#ifdef WITHMERIC
		MERIC_MeasureStop("compute");
		#endif
	}

	#ifdef WITHMERIC
	MERIC_Close();
	#endif

	return 0;
}
