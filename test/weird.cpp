
#include "hdeem.h"
#include <cstdlib>
#include <iostream>

int main(int argc, char **argv)
{
	hdeem_bmc_data_t bmc;
	timespec time;

	hdeem_init(&bmc);
	hdeem_start(&bmc);

	hdeem_stats_reading_t stats;
	hdeem_get_stats(&bmc, &stats);

	std::cout << "hdeem start: " << stats.read_time_blade.tv_sec << "\n";
	clock_gettime(CLOCK_REALTIME, &time);
	std::cout << "clock start: " << time.tv_sec << "\n";

	hdeem_stats_free(&stats);

	size_t sum = 0;
	for (size_t i = 0; i < 1e10; i++) {
		sum += i;
	}

	hdeem_stop(&bmc);


	hdeem_get_stats(&bmc, &stats);

	std::cout << "hdeem end: " << stats.read_time_blade.tv_sec << "\n";
	clock_gettime(CLOCK_REALTIME, &time);
	std::cout << "clock end: " << time.tv_sec << "\n";

	hdeem_close(&bmc);

	return 0;
}



