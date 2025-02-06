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

	MERIC_MeasureStart("WORK");
#pragma omp parallel
	do_work(20.0); // cannot be sleep!! A64FX does not accumulate power samples if the HW is idle
	MERIC_MeasureStop("WORK");

	MERIC_Close();
	return 0;
}
