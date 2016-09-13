
#include "../include/meric.h"

#include <cstdlib>
#include <iostream>
#include <unistd.h>

int main(int argc, char **argv)
{
	MERIC_Init();

	MERIC_MeasureStart("A");

	MERIC_MeasureStart("B");
	MERIC_MeasureStop();

	MERIC_MeasureStart("B");

	sleep(2);

	MERIC_MeasureStop();
	MERIC_MeasureStop();

	MERIC_Close();
	return 0;
}




