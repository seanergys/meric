
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <ctime>
#include "hdeemwrapper.h"

using namespace meric;

hdeem_bmc_data_t HDEEM::bmc;

static void checkHDEEM(int status)
{
	if (status) {
		std::cerr << std::hex << status << "\n";
	}
}

void HDEEM::init()
{
	checkHDEEM(hdeem_init(&bmc));
}

void HDEEM::close()
{
	hdeem_close(&bmc);
}

void HDEEM::start()
{
	checkHDEEM(hdeem_start(&bmc));
}

void HDEEM::stop()
{
	checkHDEEM(hdeem_stop(&bmc));
}

void HDEEM::fillEnergy(double &energy, timespec &time)
{
	hdeem_stats_reading_t stats;
	checkHDEEM(hdeem_get_stats(&bmc, &stats));
	energy = stats.energy_blade_values.value[0];
	time = stats.read_time_blade;
}




