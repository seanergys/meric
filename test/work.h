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
	struct timespec startTime, endTime;
	clock_gettime(CLOCK_MONOTONIC, &startTime);
	double start = timespec2double(startTime);

	while(1)
	{
		
		for(i = 0; i < REPS; i++)
			l += l * i;

		clock_gettime(CLOCK_MONOTONIC, &endTime);
		exe_time = timespec2double(endTime) - start;

		if(exe_time > timeout)
			break;
	}

}
