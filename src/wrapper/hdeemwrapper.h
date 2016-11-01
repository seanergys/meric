
#ifndef SRC_WRAPPER_HDEEMWRAPPER_H_
#define SRC_WRAPPER_HDEEMWRAPPER_H_

#include "hdeem.h"
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <ctime>
#include <array>
#include <unistd.h>

#define PROBE_NUMBER 7

namespace meric {

#ifndef USE_DUMMY

class HDEEM {

public:
	static void init() { checkHDEEM(hdeem_init(&bmc), "init"); }
	static void close() { hdeem_close(&bmc); }
	static void start() 
	{
		int status = hdeem_start(&bmc);
		if (status)
		{
			checkHDEEM(status, "start");
		}
		else	//second start according to hdeem reference guide
		{
			checkHDEEM(hdeem_clear(&bmc), "clear");
			status = hdeem_start(&bmc);
			if (status)
			{
				checkHDEEM(status, "start");
				exit(99);
			}
		}
	}
	
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

#else

class HDEEM {

public:
	static void init() {};
	static void close() {};
	static void start() {};
	static void stop() {};

	static void fillTime(timespec &time) { clock_gettime(CLOCK_REALTIME, &time); }
	static void fillEnergy(double &energy, timespec &time)
	{
		energy = HDEEM::energy++;
		clock_gettime(CLOCK_REALTIME, &time);
	}
	static void fillDetailedEnergy(std::array<double, PROBE_NUMBER> &energy, timespec &time)
	{
		for (size_t i = 0; i < PROBE_NUMBER; i++) {
			energy[i] = HDEEM::energy;
		}
		HDEEM::energy++;
		clock_gettime(CLOCK_REALTIME, &time);
	}

	static void getStatus(hdeem_status_t &status)
	{
		status.status = 0;
		status.total_blade_values = 0;
		status.total_vr_values = 0;
		status.pending_blade_values = 0;
		status.pending_vr_values = 0;
		clock_gettime(CLOCK_REALTIME, &status.start_time_blade);
		clock_gettime(CLOCK_REALTIME, &status.start_time_vr);
		clock_gettime(CLOCK_REALTIME, &status.stop_time_blade);
		clock_gettime(CLOCK_REALTIME, &status.stop_time_vr);
	}
	static void getSamples(hdeem_global_reading_t &samples) { samples.nb_blade_values = samples.nb_vr_values = 0; }
	static void deleteSamples(hdeem_global_reading_t &samples) {};

private:
	static size_t energy;
};


#endif

}



#endif /* SRC_WRAPPER_HDEEMWRAPPER_H_ */
