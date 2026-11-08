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

#include "../basis/async_utils.h"
#include "../basis/sysfsutils.h"
#include "basewrapper.h"
#include "counters.h"

using namespace HWMON_COUNTERS;

namespace meric
{

class HWMON : public BaseWrapper
{
public:
	typedef unsigned long long int RawCounterValue_t;

	struct CounterValue_t
	{
		CounterValue_t(uint64_t value = 0) : generation(0), value(value)
		{
			if ((value & (uint64_t(1) << 63)) == 0)
			{
				return;
			}

			generation = (value & ~(uint64_t(1) << 63)) >> 32;
			this->value = value & 0xFFFFFFFF;
		}

		CounterValue_t(RawCounterValue_t value, uint64_t generation) : generation(generation), value(value) {}

		operator uint64_t() const
		{
			if (generation != 0)
			{
				return (generation << 32) | (value & 0xFFFFFFFF) | (uint64_t(1) << 63);
			}
			else
			{
				return value;
			}
		}

		uint64_t generation;
		RawCounterValue_t value;
	};

	typedef std::map<std::string, CounterValue_t> CounterRecord_t;

	// Counters interface
	static void init(bool detailed = false, double sampleFreqHz = 0.0);
	static void close();

	static void getCounterValues(CounterRecord_t &record);
	static void initCounters(bool detailed);
	static double getResultValue(CounterValue_t startValue, CounterValue_t stopValue, double runtime, int &cumulative, std::string counter,
								 std::string regionName = "");

	static std::string summaryCounter();
	static std::string getSummaryCounterName();
	static std::string getSourceLabel();
	static std::string getCounterLabel(const std::string counter);

	static void getRatios(CounterRecord_t &start, CounterRecord_t &stop, double runtime, std::map<std::string, double> &ratios) {}

	static void initCountersUnits() {}
	static double getUnit(std::string counterName);

	// Sampling iterface
	static const std::string SUM_ENERGY_LABEL;

	static void start(timespec &time, bool storeSamples = false, double sampleFreqHz = 0.0);
	static void stop(timespec &time);
	static void initSamplers(bool detailed);
	static void closeSamplers();
	static int getSamples();
	static void getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> &samples);
	static std::string getData(double startTime, double duration, SamplerRecord_t &record);
	static void setTimeAdjustment(timespec &time) {}
	static void fillTime(timespec &time) { clock_gettime(CLOCK_REALTIME, &time); }
	static void deleteSamples() {}
	static size_t getSamplerCount() { return PROBE_NUMBER; }
	static std::string getSamplerLabel(size_t id)
	{
		if (id > getSamplerCount())
		{
			return "";
		}
		return vrSensorLabel[id];
	}

	static void getSamplePeriodInfo(SamplePeriodInfo_t &info);
	static bool isAvailable();

protected:
	struct Sample_t
	{
		uint64_t timestamp;
		uint64_t value;
	};

	struct SourceDecl_t;
	struct SourceDesc_t;
	struct CollectSources_t;

	typedef std::map<std::string, std::map<int, SourceDesc_t>> SourceDescMap_t;
	typedef std::map<std::string, SourceDesc_t *> SourceNameMap_t;
	typedef std::vector<SourceDesc_t *> SourceIdVec_t;

	template <typename T, typename F>
	static void CollectSources(const std::vector<std::string> &paths, const CollectSources_t &desc, const std::map<std::string, SourceDecl_t> &sourcesDecl,
							   std::vector<T> &sourcesDef, F &&f);

	template <typename F> static void MakeSources(const SourceDecl_t &sourceDecl, F &&f);

	static int PROBE_NUMBER;
	static std::vector<std::string> vrSensorLabel;

	static const std::string s_rootPath;
	static const std::vector<std::string> s_checkLabelFiles;
	static const std::set<std::string> s_checkLabels;
	static const std::regex s_labelRegex;

	static SourceDescMap_t s_sourceDesc;
	static SourceNameMap_t s_sourceByName;
	static std::vector<SourceDesc_t *> s_sourceById;

	static std::chrono::nanoseconds s_defaultSamplePeriod;
	static std::chrono::nanoseconds s_minSamplePeriod;
	static std::chrono::nanoseconds s_currentSamplePeriod;
	static std::atomic<bool> s_sampleStoreEnabled;
	static AsyncSampler<CounterRecord_t> s_asyncSampler;
};
}

#endif // SRC_WRAPPER_HWMONWRAPPER_H_
