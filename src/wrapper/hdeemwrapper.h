
#ifndef SRC_WRAPPER_HDEEMWRAPPER_H_
#define SRC_WRAPPER_HDEEMWRAPPER_H_

#include "../basis/utils.h"
#ifdef HAVE_HDEEM_H
	#include <hdeem.h>
#else
	#include "../../hdeem/hdeem.h"
#endif
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <ctime>
#include <array>
#include <vector>
#include <map>
#include <unistd.h>
#include <cmath>
#include <climits>

namespace meric {

class HDEEM {

public:
	static const int FREQUENCY = 1000;
	static const int FREQUENCY_VR = 100;
	static const int VRDELAY = 2; //from list of VR samples we noticed, that VR samples are shifted by two samples
	static const int PROBE_NUMBER = 9;	//BLADE + {"CPU0", "CPU1", "DDR_1", "DDR_2", "DDR_3", "DDR_4", "MEZZA", "VAUX" };
	static std::vector<std::string> vrSensorLabel;	//Voltage regulator
	static std::string getCounterLabel(std::string counter) { return counter; };
	static std::string getSourceLabel() {return "HDEEM";};
	static const std::string SUM_ENERGY_LABEL;

#ifdef HAVE_HDEEM_H
	static void init(bool detailed = false)
	{
		detailedMode = detailed;
		checkHDEEM(hdeem_init(&bmc), "hdeem_init");
		checkHDEEM(hdeem_clear(&bmc), "hdeem_clear");	//extra clear should not make a problem

		vrSensorLabel.push_back(bmc.name_blade_sensors[0]);
		for (int vr = 0; vr < bmc.nb_vr_sensors; vr++)
			vrSensorLabel.push_back(bmc.name_vr_sensors[vr]);
	}
	
	static void close()
	{
		hdeem_close(&bmc); //void
	}
	
	static void start(timespec & time)
	{
		MERIC_INFO << "HDEEM START\n";
		clock_gettime(CLOCK_REALTIME, &time);
		if (checkHDEEM(hdeem_start(&bmc), "hdeem_start"))
		{
			//second start according to hdeem reference guide
			std::cerr << "HDEEM ERROR: function start (1st), trying 2nd start...\n";
			checkHDEEM(hdeem_clear(&bmc), "hdeem_clear");
			if (checkHDEEM(hdeem_start(&bmc), "hdeem_start (2nd)"))
			{
				exit(99);
			}
		}
	}
	
	static void stop(timespec & time)
	{
		MERIC_INFO << "HDEEM STOP\n";
		usleep(2e4); //extra 20 blade samples, 2 vr samples
		clock_gettime(CLOCK_REALTIME, &time);
		checkHDEEM(hdeem_stop(&bmc), "hdeem_stop");
	}

	static void getCounterValues(std::map<std::string, double> & record)	//HDEEM in counter regime - counterstore
	{
		hdeem_stats_reading_t stats;
		checkHDEEM(hdeem_get_stats(&bmc, &stats), "hdeem_get_stats");
		record[SUM_ENERGY_LABEL] = stats.energy_blade_values.value[0];
		if(detailedMode)
		{
			for (unsigned int sensorID = 1; sensorID < vrSensorLabel.size(); sensorID++)
				record[vrSensorLabel[sensorID]+" [J]"] = stats.energy_vr_values.value[sensorID-1];
		}
		hdeem_stats_free(&stats); //void
	}
	
	static void getDetailedCounterValues(std::array<double, PROBE_NUMBER> &energy)		//HDEEM in counter regime - TimeEnergyStore - TODO remove
	{
		hdeem_stats_reading_t stats;
		checkHDEEM(hdeem_get_stats(&bmc, &stats), "hdeem_get_stats");
		energy[0] = stats.energy_blade_values.value[0];
		memcpy(energy.data() + 1, stats.energy_vr_values.value, (vrSensorLabel.size() - 1) * sizeof(double));
		hdeem_stats_free(&stats);
	}

	static void setTimeAdjustment(timespec start);
	static void getStatus(hdeem_status_t &status) { checkHDEEM(hdeem_check_status(&bmc, &status), "hdeem_check_status"); }
	static int getSamples()	// HDEEM in continual regime
	{
		timespec time1, time2;
		clock_gettime(CLOCK_REALTIME, &time1);
		checkHDEEM(hdeem_get_global(&bmc, &_samples), "hdeem_get_global");
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
		hdeem_data_free(&_samples); //void
		clock_gettime(CLOCK_REALTIME, &time2);
		MERIC_INFO << "MERIC DELETE HDEEM SAMPLES: " << mutils::timeToDouble(time2 - time1)<<" s\n";
	}
	
	static std::string getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double energy[], double min[], double max[]);
	static void getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> & samples);
	
	static void getRatios(std::map<std::string, double>& start, std::map<std::string, double>& stop, double runtime, std::map <std::string, double> & ratios) {};
	static double getResultValue(double startValue, double stopValue, double runtime, std::string counter, std::string regionName);
	static std::string getSummaryCounterName();

private:
	static hdeem_bmc_data_t bmc;
	static hdeem_global_reading_t _samples;
	static size_t _timeAdjustment;
	static double _startTime;
	static bool detailedMode;

	static int checkHDEEM(int status, const std::string &function)
	{
		if (status)
		{
			std::cerr << "HDEEM ERROR: function " << function << " return status " << std::hex << status << "\n";
		}
		return status;
	}
};

#else

	static void init(bool detailed)
	{
		detailedMode = detailed;
		vrSensorLabel = {"blade", "CPU0", "CPU1", "DDR_1", "DDR_2", "DDR_3", "DDR_4", "MEZZA", "VAUX" };
	};
	static void close() {};
	static void start(timespec & time)
	{
		clock_gettime(CLOCK_REALTIME, &time);
	};
	static void stop(timespec & time)
	{
		clock_gettime(CLOCK_REALTIME, &time);
	};

	static void getCounterValues(std::map<std::string, double>& record)
	{
		record[SUM_ENERGY_LABEL] = HDEEM::energy++;
	}

	static void getDetailedCounterValues(std::array<double, PROBE_NUMBER> &energy)
	{
		for (size_t i = 0; i < vrSensorLabel.size(); i++) {
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
		status.start_time_vr = status.start_time_blade;
		status.stop_time_blade = status.start_time_blade;
		status.stop_time_vr = status.start_time_blade;
	}
	static int getSamples() {return 0; };
	static void deleteSamples() {};
	static void setTimeAdjustment(timespec start) {};
	static std::string getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double energy[], double min[], double max[])
	{
		*firstBlade = 0;
		*lastBlade  = 0;
		*firstVR    = 0;
		*lastVR     = 0;
		return "";
	}
	static void getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> samples) {};

	static void getRatios(std::map<std::string, double>& start, std::map<std::string, double>& stop, double runtime, std::map <std::string, double> & ratios) {};
	static double getResultValue(double startValue, double stopValue, double runtime, std::string counter, std::string regionName) {return stopValue - startValue;};
	static std::string getSummaryCounterName();

private:
	static size_t energy;
	static bool detailedMode;
};


#endif

}



#endif /* SRC_WRAPPER_HDEEMWRAPPER_H_ */
