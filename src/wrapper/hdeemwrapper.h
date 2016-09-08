
#ifndef SRC_WRAPPER_HDEEMWRAPPER_H_
#define SRC_WRAPPER_HDEEMWRAPPER_H_

#include "hdeem.h"
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <ctime>
#include <array>

#define PROBE_NUMBER 7

namespace meric {

class HDEEM {

public:
	static void init() { checkHDEEM(hdeem_init(&bmc)); }
	static void close() { hdeem_close(&bmc); }
	static void start() { checkHDEEM(hdeem_start(&bmc)); }
	static void stop() { checkHDEEM(hdeem_stop(&bmc)); }

	static void fillTime(timespec &time) { clock_gettime(CLOCK_REALTIME, &time); }
	static void fillEnergy(double &energy, timespec &time)
	{
		hdeem_stats_reading_t stats;
		checkHDEEM(hdeem_get_stats(&bmc, &stats));
		energy = stats.energy_blade_values.value[0];
		time = stats.read_time_blade;
	}
	static void fillDetailedEnergy(std::array<double, PROBE_NUMBER> &energy, timespec &time)
	{
		hdeem_stats_reading_t stats;
		checkHDEEM(hdeem_get_stats(&bmc, &stats));
		energy[0] = stats.energy_blade_values.value[0];
		memcpy(energy.data() + 1, stats.energy_vr_values.value, (PROBE_NUMBER - 1) * sizeof(double));
		time = stats.read_time_blade;
	}

private:
	static hdeem_bmc_data_t bmc;

	static void checkHDEEM(int status)
	{
		if (status) {
			std::cerr << std::hex << status << "\n";
		}
	}
};

}



#endif /* SRC_WRAPPER_HDEEMWRAPPER_H_ */
