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
#include "../basis/async_utils.h"

using namespace HWMON_COUNTERS;

namespace meric {

class HWMON {
public:
	typedef unsigned long long int RawCounterValue_t;
	
	struct CounterValue_t {
		CounterValue_t(RawCounterValue_t value = 0)
			: value(value)
		{}

		CounterValue_t(RawCounterValue_t value, uint64_t generation)
			: generation(generation)
			, value(value)
		{}

		uint64_t generation;
		RawCounterValue_t value;
	};

	typedef std::map<std::string, CounterValue_t> CounterRecord_t;

	// Counters interface
	static void init(bool detailed);
	static void close();

	static void getCounterValues(CounterRecord_t &record);
	static void initCounters();
	static double getResultValue(CounterValue_t startValue, CounterValue_t stopValue, 
		double runtime, int &cumulative, std::string counter, std::string regionName = "");

	static std::string summaryCounter();
	static std::string getSummaryCounterName();
	static std::string getSourceLabel();
	static std::string getCounterLabel(const std::string counter);

	static void getRatios(CounterRecord_t &start, CounterRecord_t &stop, 
		double runtime, std::map<std::string, double> &ratios) {}

	static void initCountersUnits() {}
	static double getUnit(std::string counterName);

	// Sampling iterface
	static const std::string SUM_ENERGY_LABEL;
	static int PROBE_NUMBER;
	static std::vector<std::string> vrSensorLabel;

	static void start(timespec &time, double sampleFreqHz = 0.0);
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
	struct Sample_t {
		uint64_t timestamp;
		uint64_t value;
	};

	struct SourceDesc_t;
	struct CollectSourcesDef_t;

	template<typename T, typename F>
	static void CollectSources(const std::vector<std::string> &paths, const CollectSourcesDef_t &desc, std::vector<T> &counters, F &&f);

	static std::map<std::string, SourceDesc_t> s_sourceDesc;
	static std::vector<SourceDesc_t *>         s_sourceById;

	static std::chrono::nanoseconds      s_overflowCheckPeriod;
	static std::atomic<bool>		     s_sampleStoreEnabled;
	static AsyncSampler<CounterRecord_t> s_asyncSampler;
};
}

#endif // SRC_WRAPPER_HWMONWRAPPER_H_
