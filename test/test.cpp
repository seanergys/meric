#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include <meric.h>
#include "work.h"

void help(const std::string name, const int ret)
{
	std::cerr << "OpenMP application of four MERIC regions\n"
			  << " - SLEEP (idle)\n"
			  << " - NESTED (idle, nested in SLEEP region)\n"
			  << " - RUN (compute bound)\n"
			  << " - RAII (example of RAII usage, compute bound)\n"
			  << "Usage: " << name << " [-s sleep_ms] [-r run_ms] [-n nested_ms] [-a raii_ms]\n";
	exit(ret);
}

//RAII supported by Clang and GNU compilers, so the following region will not be available if using another compiler
void raiiTest(double time)
{
	MERIC_CaptureScope("RAII");
	do_work(time);
}

int main(int argc, char **argv)
{
	int sleep_ms = 1000;
	int run_ms = 1000;
	int nested_ms = 500;
	int raii_ms = 1000;

	int opt;
	while ((opt = getopt(argc, argv, "hs:r:n:a:")) != -1)
	{
		switch (opt)
		{
		case 'h':
			help(std::string(argv[0]), 0);
		case 's':
			sleep_ms = atoi(optarg);
			break;
		case 'r':
			run_ms = atoi(optarg);
			break;
		case 'n':
			nested_ms = atoi(optarg);
			break;
		case 'a':
			raii_ms = atoi(optarg);
			break;
		default:
			help(std::string(argv[0]), 1);
		}
	}

	MERIC_Init();

	MERIC_MeasureStart("SLEEP");
	usleep(sleep_ms * 1000);

	MERIC_MeasureStart("NESTED");
	usleep(nested_ms * 1000);
	MERIC_MeasureStop("NESTED");

	MERIC_MeasureStop("SLEEP");

	MERIC_MeasureStart("RUN");

#pragma omp parallel
	{
		do_work(run_ms / 1000.0);
	}

	MERIC_MeasureStop("RUN");

	raiiTest(raii_ms / 1000.0);

	MERIC_Close();

	return 0;
}
