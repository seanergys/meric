
#include "hdeem.h"
#include <stdio.h>

int main(int argc, char **argv)
{
	hdeem_bmc_data_t bmc;
	timespec time;

	hdeem_init(&bmc);
	hdeem_start(&bmc);

	hdeem_stats_reading_t stats;
	hdeem_get_stats(&bmc, &stats);

	printf("hdeem start: %ld\n", stats.read_time_blade.tv_sec);
	clock_gettime(CLOCK_REALTIME, &time);
	printf("clock start: %ld\n", time.tv_sec);

	hdeem_stats_free(&stats);

	long sum = 0;
	for (long i = 0; i < 1e10; i++) {
		sum += i;
	}

	hdeem_stop(&bmc);

	hdeem_get_stats(&bmc, &stats);

	printf("hdeem end: %ld\n", stats.read_time_blade.tv_sec);
	clock_gettime(CLOCK_REALTIME, &time);
	printf("clock end: %ld\n", time.tv_sec);

	hdeem_close(&bmc);

	return 0;
}



