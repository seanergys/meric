#include <cstdlib>
#include <ctime>
#include <unistd.h>

#include "work.h"

int main(int argc, char **argv)
{
	#pragma omp parallel
	{
		sleep(3);
		do_work(1);
	}

	return 0;
}
