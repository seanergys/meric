#include <cstdlib>
#include <ctime>
#include <iostream>
#include <math.h>
#include <omp.h>
#include <unistd.h>

#include "work.h"
#include <meric.h>

int main(int argc, char **argv)
{
	MERIC_Init();

	MERIC_MeasureStart("WORK");
#pragma omp parallel
	do_work(20.0); // cannot be sleep!! A64FX does not accumulate power samples if the HW is idle
	MERIC_MeasureStop("WORK");

	MERIC_Close();
	return 0;
}
