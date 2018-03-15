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
		double seed = time(NULL);
		READEX_REGION_START(A, "A", SCOREP_USER_REGION_TYPE_COMMON);	
#if   defined LOAD_MIN
			sleep(2);
#elif defined LOAD_MAX
			for (int k=0; k<100000000; k++)
				for (int j=0; j<100000000; j++)
					random = seed*1095 + log(random);
#endif
		READEX_REGION_START(B, "B", SCOREP_USER_REGION_TYPE_COMMON);
			sleep(2);
		READEX_REGION_STOP(B);
		
		READEX_REGION_STOP(A);
	}

	READEX_CLOSE();
	return 0;
}




