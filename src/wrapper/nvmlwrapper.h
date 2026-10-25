
#ifndef SRC_WRAPPER_NVMLWRAPPER_H_
#define SRC_WRAPPER_NVMLWRAPPER_H_

#include <map>
#include <vector>
#include <string>

#ifdef HAVE_NVML_H
#include <nvml.h>
#else
#define nvmlDevice_t void
#endif // HAVE_NVML

#include "../basis/async_utils.h"
#include "basewrapper.h"

namespace meric
{
class NVML : public BaseWrapper
{
public:
	typedef unsigned long long int CounterValue_t;
	typedef std::map<std::string, CounterValue_t> CounterRecord_t;

	// Counters Interface
	static void init(bool detailed = false, double sampleFreqHz = 0.0);
	static void close();

	static void getCounterValues(CounterRecord_t &record);
	static void initCounters(bool detailed);
	static double getResultValue(CounterValue_t startValue, CounterValue_t stopValue, double runtime, int &cumulative, std::string counter,
								 std::string regionName = "");

	static std::string summaryCounter();
	static std::string getSummaryCounterName();
	static std::string getSourceLabel();
	static std::string getCounterLabel(const std::string &counter);

	static void getRatios(CounterRecord_t &start, CounterRecord_t &stop, double runtime, std::map<std::string, double> &ratios) {}

	static void initCountersUnits() {}
	static double getUnit(std::string counterName);

	// Sampling Interface
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
			return "";

		return vrSensorLabel[id];
	}

	static void getSamplePeriodInfo(SamplePeriodInfo_t &info);

	static bool isAvailable();

	// NVML Specific
	static nvmlDevice_t getGPUHandle(int id);
	static size_t getGPUsPerNode();

protected:
	struct Sample_t
	{
		uint64_t timestamp;
		uint64_t value;
	};

	struct SourceDecl_t;
	struct SourceDesc_t;
	struct DeviceDesc_t;

	typedef std::map<std::string, std::map<int, SourceDesc_t>> SourceDescMap_t;
	typedef std::map<std::string, SourceDesc_t *> SourceNameMap_t;
	typedef std::vector<SourceDesc_t *> SourceIdVec_t;
	typedef std::map<int, DeviceDesc_t> DeviceDescMap_t;

	static int PROBE_NUMBER;
	static std::vector<std::string> vrSensorLabel;

	static SourceDescMap_t s_sourceDesc;
	static SourceNameMap_t s_sourceByName;
	static SourceIdVec_t s_sourceById;
	static DeviceDescMap_t s_deviceDesc;

	static std::chrono::nanoseconds s_defaultSamplePeriod;
	static std::chrono::nanoseconds s_minSamplePeriod;
	static std::chrono::nanoseconds s_currentSamplePeriod;
	static std::atomic<bool> s_sampleStoreEnabled;
	static AsyncSampler<CounterRecord_t> s_asyncSampler;
};
}
#endif /* SRC_WRAPPER_NVMLWRAPPER_H_ */
