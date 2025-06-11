#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <ctime>
#include <math.h>
#include <omp.h>

#include <meric.h>
#include "work.h"


int main(int argc, char **argv)
{
	MERIC_Init();

	// double time1;
	// double random = 991611579.454984;
	// timespec spectime1, spectime2;


	// MERIC_MeasureStart("RUN");
	//  	clock_gettime(CLOCK_REALTIME, &spectime1);
	//  	time1 = timespec2double(spectime1);
	//  	#pragma omp parallel private(spectime2)
	// 	do
	// 	{
	// 		random = time1*1095 + log(random);
	//  		clock_gettime(CLOCK_REALTIME, &spectime2);
	// 	}while(timespec2double(spectime2)-time1 < 5.0);
	// MERIC_MeasureStop("RUN");

	
	// #pragma omp parallel
	// {
		MERIC_MeasureStart("SLEEP");
			sleep(5);
			// usleep(50000);
		MERIC_MeasureStop("SLEEP");
	// }
	
	MERIC_Close();
	return 0;
}
