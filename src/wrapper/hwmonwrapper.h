#ifndef SRC_WRAPPER_HWMONWRAPPER_H_
#define SRC_WRAPPER_HWMONWRAPPER_H_

#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include "counters.h"
#include "../basis/sysfsutils.h"

using namespace HWMON_COUNTERS;

namespace meric {

class HWMON {
public:
	static void init(bool detailed);
	static void close();
	static void getCounterValues(std::map<std::string, unsigned long long int> &record);
	static void initMap(std::map<std::string, unsigned long long int> &dict);
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

protected:
	struct CounterDesc_t {
		double unit;
		std::string path;
		std::ifstream fs;
		std::string label;
		bool isCumulative;
	};

	static std::map<std::string, CounterDesc_t> s_counterDesc;
	static std::map<std::string, unsigned long long int> counters;
};
}

#endif // SRC_WRAPPER_HWMONWRAPPER_H_
