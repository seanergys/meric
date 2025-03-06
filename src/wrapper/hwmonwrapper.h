#ifndef SRC_WRAPPER_HWMONWRAPPER_H_
#define SRC_WRAPPER_HWMONWRAPPER_H_

#include <cstdint>
#include <ctime>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <regex>
#include <sstream>
#include <set>
#include <cmath>
#include <dirent.h>

#include "counters.h"
#include "../basis/sysfsutils.h"

using namespace HWMON_COUNTERS;

namespace meric {

class HWMON {
public:
	static void init(bool detailed);
	static void close();
	static void getCounterValues(std::map<std::string, unsigned long long int> &record);
	static void initCounters();
	static double getResultValue(unsigned long long int startValue, unsigned long long int stopValue, 
		double runtime, int &cumulative, std::string counter, std::string regionName = "");
	static std::string summaryCounter();
	static std::string getSummaryCounterName();
	static std::string getSourceLabel();
	static std::string getCounterLabel(const std::string counter);
	static void getRatios(std::map<std::string, unsigned long long int> &start, 
		std::map<std::string, unsigned long long int> &stop, double runtime,
		std::map<std::string, double> &ratios) {}
	static void initCountersUnits() {}
	static double getUnit(std::string counterName);

	// Sampling
	static const std::string SUM_ENERGY_LABEL;
	static int PROBE_NUMBER;
	static std::vector<std::string> vrSensorLabel;

	static void start(timespec &time);
	static void stop(timespec &time);
	static void initSamplers();
	static void closeSamplers();
	static int getSamples();
	static void getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> &samples);
	static std::string getData(double startTime, double duration, 
		size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, 
		double energy[], double minWatts[], double maxWatts[]);
	static void setTimeAdjustment(timespec &time) {}
	static void fillTime(timespec &time)  { clock_gettime(CLOCK_REALTIME, &time); }
	static void deleteSamples() {}

protected:
	struct CounterDesc_t {
		double unit;
		std::string path;
		std::ifstream fs;
		std::string label;
		bool isCumulative;
	};

	struct Sample_t {
		uint64_t timestamp;
		uint64_t value;
	};

	struct SamplerDesc_t {
		size_t id;
		double unit;
		std::string path;
		std::ifstream fs;
		std::string label;
		bool isCumulative;
		uint64_t period;
		std::vector<Sample_t> samples;
	};

	static std::map<std::string, CounterDesc_t> s_counterDesc;
	static std::map<std::string, SamplerDesc_t> s_samplerDesc;
	static std::vector<SamplerDesc_t *>         s_samplerById;
	static uint64_t s_samplingPeriod;
	static std::thread s_samplingThread;
	static std::atomic<bool> s_samplingEnabled;
};
}

#endif // SRC_WRAPPER_HWMONWRAPPER_H_
