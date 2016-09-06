
#include "mpi.h"
#include "../include/meric.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	MERIC_Init();

	size_t sum = 0;
	for (size_t i = 0; i < 10000; i++) {
		sum += i;
	}
	std::cout << "SUM: " << sum << "\n";

	MERIC_Close();

	MPI_Finalize();
	return 0;
}



