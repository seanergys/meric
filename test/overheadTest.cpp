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

void doSomeJobForMe(double random)
{
	double seed = time(NULL);
	sleep(1);
	for (int k=0; k<100000000; k++)
		for (int j=0; j<100000000; j++)
		{
        		random = seed*1095 + log(random);
			random = std::tan(0.333*k*j*(random+1));
		}
}

int main(int argc, char **argv)
{
	READEX_INIT();

	READEX_REGION_DEFINE(A);
        READEX_REGION_DEFINE(B);

	srand (time(NULL));

	#pragma omp parallel
	for (int i=0; i<100; i++)
	{
		double random = (double)rand() / RAND_MAX;
		READEX_REGION_START(A, "A", SCOREP_USER_REGION_TYPE_COMMON);	
			doSomeJobForMe(random);
			READEX_REGION_START(B, "B", SCOREP_USER_REGION_TYPE_COMMON);
				doSomeJobForMe(random);
			READEX_REGION_STOP(B);
		READEX_REGION_STOP(A);
	}

	READEX_CLOSE();
	return 0;
}




