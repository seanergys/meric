
#include <readex.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>

int main(int argc, char **argv)
{
	READEX_INIT();

	READEX_REGION_DEFINE(SLEEP);
#pragma omp parallel
{
	sleep(1);
	READEX_REGION_START(SLEEP, "SLEEP", SCOREP_USER_REGION_TYPE_COMMON);
	sleep(5);

	READEX_REGION_STOP(SLEEP);
}
	READEX_CLOSE();
	return 0;
}




