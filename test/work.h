//#define _GNU_SOURCE
/*
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
*/
#include <sys/time.h>
#define REPS 100000

double timespec2double(timespec time) 
{
	return time.tv_sec + (double)time.tv_nsec/1e9;
}

void do_work(double timeout)
{
	int i, l = 1;
	double exe_time = 0;
	struct timespec start, end;

	while(1)
	{
		clock_gettime(CLOCK_MONOTONIC, &start);
		for(i = 0; i < REPS; i++)
			l += l * i;

		clock_gettime(CLOCK_MONOTONIC, &end);
		exe_time += timespec2double(end) - timespec2double(start);

		if(exe_time > timeout)
			break;
	}

}
