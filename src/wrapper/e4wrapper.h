
#ifndef SRC_WRAPPER_E4WRAPPER_H_
#define SRC_WRAPPER_E4WRAPPER_H_

#include "../basis/spline.h"
#include "../basis/utils.h"
#include "../basis/async_utils.h"
#include "basewrapper.h"
#include <iostream>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <sstream>
#include <map>
#include <algorithm>

#if HAVE_E4_PDU
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#endif

namespace meric
{
	class E4 : public BaseWrapper
	{
	public:
		typedef std::map<std::string, unsigned long long> CounterRecord_t;

		static void init(std::string label, bool detailed = false, double sampleFreqHz = 0.0);
		static void close();

		static void getCounterValues(CounterRecord_t &record);
		static void initMap(std::map<std::string, unsigned long long> &dict) {} // not needed
		static double getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, int &cumulative, std::string counter, std::string regionName = "");
		static std::string getSummaryCounterName() { return "Energy consumption [J]"; }
		static std::string getSourceLabel() { return "E4"; };
		static std::string getCounterLabel(std::string counter) { return "E4_" + counter + " [J]"; }
		static void getRatios(std::map<std::string, unsigned long long> &start, std::map<std::string, unsigned long long> &stop, double runtime, std::map<std::string, double> &ratios) {};

		// E4 samples are stored in Watts.
		static inline double getUnit(std::string counterName)
		{
			(void)counterName;
			return 1.0;
		};

		// Sampling Interface
		static const std::string SUM_ENERGY_LABEL;

		static void start(timespec &time, bool storeSamples = false, double sampleFreqHz = 0.0);
		static void stop();
		static void initSamplers(bool detailed);
		static void closeSamplers();
		static int getSamples();
		static void getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> &samples);
		static std::string getData(double startTime, double duration, SamplerRecord_t &record);
		static void setTimeAdjustment(timespec &time) {}
		static void fillTime(timespec &time) { clock_gettime(CLOCK_REALTIME, &time); }
		static void deleteSamples() {}
		static size_t getSamplerCount() { return PROBE_NUMBER; }
        static std::string getSamplerLabel(size_t id) { 
            if(id > getSamplerCount())
                return "";
            
            return vrSensorLabel[id]; 
        }


	static std::string powerDevice;
	static bool isAvailable();

	private:
		struct Sample_t
		{
			uint64_t timestamp;
			double value;
		};

		struct Sampler_t
		{
			std::string name;
			double totalpower;
			std::vector<Sample_t> samples;
		};

		static int PROBE_NUMBER;
		static std::vector<std::string> vrSensorLabel;

		static std::chrono::nanoseconds s_defaultSamplePeriod;
        static std::chrono::nanoseconds s_minSamplePeriod;
        static std::chrono::nanoseconds s_currentSamplePeriod;
		static std::atomic<bool> s_sampleStoreEnabled;
		static AsyncSampler<CounterRecord_t> s_asyncSampler;

		static std::map<std::string, Sampler_t> samplers;
		static std::vector<Sampler_t *> samplersById;
	};

}

#endif /* SRC_WRAPPER_E4WRAPPER_H_ */