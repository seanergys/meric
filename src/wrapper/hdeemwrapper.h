
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
	static void init() { checkHDEEM(hdeem_init(&bmc), "init"); }
	static void close() { hdeem_close(&bmc); }
	static void start() { checkHDEEM(hdeem_start(&bmc), "start"); }
	static void stop() { checkHDEEM(hdeem_stop(&bmc), "stop"); }

	static void fillTime(timespec &time) { clock_gettime(CLOCK_REALTIME, &time); }
	static void fillEnergy(double &energy, timespec &time)
	{
		hdeem_stats_reading_t stats;
		checkHDEEM(hdeem_get_stats(&bmc, &stats), "get_stats");
		energy = stats.energy_blade_values.value[0];
		time = stats.read_time_blade;
		hdeem_stats_free(&stats);
	}
	static void fillDetailedEnergy(std::array<double, PROBE_NUMBER> &energy, timespec &time)
	{
		hdeem_stats_reading_t stats;
		checkHDEEM(hdeem_get_stats(&bmc, &stats), "get_stats");
		energy[0] = stats.energy_blade_values.value[0];
		memcpy(energy.data() + 1, stats.energy_vr_values.value, (PROBE_NUMBER - 1) * sizeof(double));
		time = stats.read_time_blade;
		hdeem_stats_free(&stats);
	}

	static void getStatus(hdeem_status_t &status) { checkHDEEM(hdeem_check_status(&bmc, &status), "get_status"); }
	static void getSamples(hdeem_global_reading_t &samples) { checkHDEEM(hdeem_get_global(&bmc, &samples), "get_global"); }
	static void deleteSamples(hdeem_global_reading_t &samples) { hdeem_data_free(&samples); }

private:
	static hdeem_bmc_data_t bmc;

	static void checkHDEEM(int status, const std::string &function)
	{
		if (status) {
			std::cerr << "HDEEM ERROR: function " << function << " return status " << std::hex << status << "\n";
		}
	}
};

}



#endif /* SRC_WRAPPER_HDEEMWRAPPER_H_ */
