
#include <readex.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <ctime>
#include <math.h>

#include <omp.h>

double timespec2double(timespec time) 
{
	return time.tv_sec + (double)time.tv_nsec/1e9;
}


int main(int argc, char **argv)
{
	READEX_INIT();

	double time1;
	double random = 991611579.454984;
	timespec spectime1, spectime2;

	READEX_REGION_DEFINE(SLEEP);
	// READEX_REGION_DEFINE(RUN);

	// READEX_REGION_START(RUN, "RUN", SCOREP_USER_REGION_TYPE_COMMON);
	// 	clock_gettime(CLOCK_REALTIME, &spectime1);
	// 	time1 = timespec2double(spectime1);
	// 	#pragma omp parallel private(spectime2)
	// 		do
	// 		{
	// 			random = time1*1095 + log(random);
	// 			clock_gettime(CLOCK_REALTIME, &spectime2);
	// 		}while(timespec2double(spectime2)-time1 < 5.0);
	// READEX_REGION_STOP(RUN);

	
	// #pragma omp parallel
	// {
		READEX_REGION_START(SLEEP, "SLEEP", SCOREP_USER_REGION_TYPE_COMMON);
			//sleep(1);
			usleep(50000);
		READEX_REGION_STOP(SLEEP);
	// }
	
	READEX_CLOSE();
	return 0;
}




