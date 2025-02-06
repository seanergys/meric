#include <stdio.h>
#include <time.h>
#define REPS 100000

// Function to convert timespec to a double value (in seconds)
double timespec2double(struct timespec time) 
{
    return time.tv_sec + (double)time.tv_nsec / 1e9;
}

// Function to perform work for a specified timeout duration
void do_work(double timeout)
{
    int i, l = 1;
    double exe_time = 0.0;
    struct timespec startTime, endTime;

    // Get the starting time
    clock_gettime(CLOCK_MONOTONIC, &startTime);
    double start = timespec2double(startTime);

    // Loop until the elapsed time exceeds the timeout
    while (1)
    {
        // Perform some dummy work
        for (i = 0; i < REPS; i++)
            l += l * i;

        // Get the current time
        clock_gettime(CLOCK_MONOTONIC, &endTime);

        // Calculate the elapsed time
        exe_time = timespec2double(endTime) - start;

        // Break if the timeout is reached
        if (exe_time > timeout)
            break;
    }
}