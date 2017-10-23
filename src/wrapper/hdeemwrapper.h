
#ifndef SRC_WRAPPER_HDEEMWRAPPER_H_
#define SRC_WRAPPER_HDEEMWRAPPER_H_

#include "../basis/utils.h"
#ifdef USE_DUMMY
	#include "../../hdeem/hdeem.h"
#else
	#include <hdeem.h>
#endif
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <ctime>
#include <array>
#include <vector>
#include <unistd.h>
#include <cmath>

//#define PROBE_NUMBER 7

namespace meric {


class HDEEM {

public:
	static const int FREQUENCY = 1000;
	static const int FREQUENCY_VR = 100;
	static const int VRDELAY = 2; //from list of VR samples we noticed, that VR samples are shifted by two samples
	static const int PROBE_NUMBER = 7;	//BLADE + {"CPU0", "CPU1", "DDR AB", "DDR CD", "DDR EF", "DDR GH"};
	static std::string storeLabel;
	static std::vector<std::string> vrSensorLabel;	//Voltage regulator

#ifndef USE_DUMMY
	static void init()
	{
		checkHDEEM(hdeem_init(&bmc), "init");
		checkHDEEM(hdeem_clear(&bmc), "clear");	//extra clear should not make a problem
	}
	
	static void close() { hdeem_close(&bmc); }
	static void start(timespec & time)
	{
		MERIC_INFO << "HDEEM START\n";
		clock_gettime(CLOCK_REALTIME, &time);
		if (hdeem_start(&bmc))
		{
			//second start according to hdeem reference guide
			std::cerr << "HDEEM ERROR: function start (1st), trying 2nd start...\n";
			checkHDEEM(hdeem_clear(&bmc), "clear");
			int status = hdeem_start(&bmc);
			if (status)
			{
				checkHDEEM(status, "start (2nd)");
				exit(99);
			}
		}
	}
	
	static void stop(timespec & time)
	{
		MERIC_INFO << "HDEEM STOP\n";
		usleep(2e4); //extra 20 blade samples, 2 vr samples
		clock_gettime(CLOCK_REALTIME, &time);
		checkHDEEM(hdeem_stop(&bmc), "stop");
	}

	static void getCounterValues(double &energy)	//HDEEM in counter regime
	{
		hdeem_stats_reading_t stats;
		checkHDEEM(hdeem_get_stats(&bmc, &stats), "get_stats");
		energy = stats.energy_blade_values.value[0];
		hdeem_stats_free(&stats);
	}
	static void getDetailedCounterValues(std::array<double, PROBE_NUMBER> &energy)		//HDEEM in counter regime
	{
		hdeem_stats_reading_t stats;
		checkHDEEM(hdeem_get_stats(&bmc, &stats), "get_stats");
		energy[0] = stats.energy_blade_values.value[0];
		memcpy(energy.data() + 1, stats.energy_vr_values.value, (PROBE_NUMBER - 1) * sizeof(double));
		hdeem_stats_free(&stats);
	}

	static void setTimeAdjustment(timespec start);
	static void getStatus(hdeem_status_t &status) { checkHDEEM(hdeem_check_status(&bmc, &status), "get_status"); }
	static int getSamples()	// HDEEM in continual regime
	{
		timespec time1, time2;
		clock_gettime(CLOCK_REALTIME, &time1);
		checkHDEEM(hdeem_get_global(&bmc, &_samples), "get_global");
		clock_gettime(CLOCK_REALTIME, &time2);
		MERIC_INFO << "MERIC GET SAMPLES: " << mutils::timeToDouble(time2 - time1) <<" s\n";

		if(_samples.nb_blade_values == 0)
		{
			std::cerr << "MERIC ERROR: No HDEEM samples\n";
			deleteSamples();
			return 1;
		}
		else
			return 0;
	}
	
	static void deleteSamples()	//HDEEM in continual regime
	{
		timespec time1, time2;
		clock_gettime(CLOCK_REALTIME, &time1);
		hdeem_data_free(&_samples);
		clock_gettime(CLOCK_REALTIME, &time2);
		MERIC_INFO << "MERIC DELETE HDEEM SAMPLES: " << mutils::timeToDouble(time2 - time1)<<" s\n";
	}
	
	static std::string getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double energy[], double min[], double max[]);
	static void getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> & samples);

private:
	static hdeem_bmc_data_t bmc;
	static hdeem_global_reading_t _samples;
	static size_t _timeAdjustment;
	static double _startTime;

	static void checkHDEEM(int status, const std::string &function)
	{
		if (status) {
			std::cerr << "HDEEM ERROR: function " << function << " return status " << std::hex << status << "\n";
		}
	}
};

#else

	static void init() {};
	static void close() {};
	static void start(timespec & time)
	{
		clock_gettime(CLOCK_REALTIME, &time);
	};
	static void stop(timespec & time)
	{
		clock_gettime(CLOCK_REALTIME, &time);
	};

	static void getCounterValues(double &energy)
	{
		energy = HDEEM::energy++;
	}
	static void getDetailedCounterValues(std::array<double, PROBE_NUMBER> &energy)
	{
		for (size_t i = 0; i < PROBE_NUMBER; i++) {
			energy[i] = HDEEM::energy;
		}
		HDEEM::energy++;
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
	static int getSamples() {return 0; };
	static void deleteSamples() {};
	static void setTimeAdjustment(timespec start) {};
	static std::string getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double energy[], double min[], double max[]) { return "";};	//TODO mozna bude potreba inicializovat zde vstupni parametry
	static void getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> samples) {};

private:
	static size_t energy;
};


#endif

}



#endif /* SRC_WRAPPER_HDEEMWRAPPER_H_ */
