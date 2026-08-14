#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <ctime>
#include <math.h>

#include <meric.h>
#include "work.h"


int main(int argc, char **argv)
{
	MERIC_Init();

	MERIC_MeasureStart("SLEEP");
		sleep(5);
	MERIC_MeasureStop("SLEEP");
	
	MERIC_Close();
	return 0;
}
