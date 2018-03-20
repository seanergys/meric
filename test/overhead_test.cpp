/*
 * in this test we watch how much time is necessary to change from the region A settings to settings for region B
 * one must specify region.options for region A and B to switch 
 **/
#include <readex.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <chrono>
#include <ctime>

void doSomeJobForMe(double random)
{
	double seed = time(NULL);
	sleep(1);
	for (int k=0; k<10; k++)
		for (int j=0; j<10; j++)
		{
//        		random = seed*1095 + log(random);
			random = std::tan(0.333*k*j*(random+1));
		}

}

double timespec2double(timespec time) 
{
	return time.tv_sec + (double)time.tv_nsec/1e6;
}

int main(int argc, char **argv)
{
	timespec spectime1, spectime2;
	
	READEX_INIT();

	READEX_REGION_DEFINE(A);
        READEX_REGION_DEFINE(B);

	srand (time(NULL));

	for (int i=0; i<100; i++)
	{
		double random = (double)rand() / RAND_MAX;
		READEX_REGION_START(A, "A", SCOREP_USER_REGION_TYPE_COMMON);
			doSomeJobForMe(random);
			clock_gettime(CLOCK_REALTIME, &spectime1);
			READEX_REGION_START(B, "B", SCOREP_USER_REGION_TYPE_COMMON);
				clock_gettime(CLOCK_REALTIME, &spectime2);
				std::cout << "MERIC start time " << (timespec2double(spectime2)-timespec2double(spectime1)) << " ms\n";
				doSomeJobForMe(random);
				clock_gettime(CLOCK_REALTIME, &spectime1);
			READEX_REGION_STOP(B);
			clock_gettime(CLOCK_REALTIME, &spectime2);
			std::cout << "MERIC stop time " << (timespec2double(spectime2)-timespec2double(spectime1)) << " ms\n";
		READEX_REGION_STOP(A);
		doSomeJobForMe(random);
	}

	READEX_CLOSE();
	return 0;
}




