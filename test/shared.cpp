
#include "../include/meric.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char **argv)
{
	MERIC_Init();

	MERIC_MeasureStart("A");

	MERIC_MeasureStart("B");
	MERIC_MeasureStop();

	MERIC_MeasureStart("B");

	size_t sum = 0;
	for (size_t i = 0; i < 1e10; i++) {
		sum += i;
	}

	MERIC_MeasureStop();
	MERIC_MeasureStop();

	MERIC_Close();
	return 0;
}




