
#include "../include/meric.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char **argv)
{
	MERIC_Init();

	size_t sum = 0;
	for (size_t i = 0; i < 10000; i++) {
		sum += i;
	}
	std::cout << "SUM: " << sum << "\n";

	MERIC_Close();
	return 0;
}




