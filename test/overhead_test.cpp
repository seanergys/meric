/*
 * in this test we watch how much time is necessary to change from the region A settings to settings for region B
 * one must specify region.options for region A and B to switch 
 **/
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <cmath>
#include <chrono>
#include <ctime>

#include <meric.h>
#include "work.h"

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


int main(int argc, char **argv)
{
	timespec spectime1, spectime2;
	MERIC_Init();
	srand (time(NULL));

	for (int i=0; i<100; i++)
	{
		double random = (double)rand() / RAND_MAX;
		MERIC_MeasureStart("A");
			doSomeJobForMe(random);
			clock_gettime(CLOCK_REALTIME, &spectime1);
			MERIC_MeasureStart("B");
				clock_gettime(CLOCK_REALTIME, &spectime2);
				std::cout << "MERIC start time " << (timespec2double(spectime2)-timespec2double(spectime1)) << " ms\n";
				doSomeJobForMe(random);
				clock_gettime(CLOCK_REALTIME, &spectime1);
			MERIC_MeasureStop("B");
			clock_gettime(CLOCK_REALTIME, &spectime2);
			std::cout << "MERIC stop time " << (timespec2double(spectime2)-timespec2double(spectime1)) << " ms\n";
		MERIC_MeasureStop("A");
		doSomeJobForMe(random);
	}

	MERIC_Close();
	return 0;
}
