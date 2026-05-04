
#ifndef SRC_WRAPPER_A64FXWRAPPER_H_
#define SRC_WRAPPER_A64FXWRAPPER_H_

#include "../basis/utils.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <ctime>
#include <vector>
#include <map>
#include <cmath>
#include <vector>
#include <map>
#include <iostream>

#ifdef HAVE_LINUX_PERF_EVENT_H
	#include <linux/perf_event.h>
#endif
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

#include "counters.h"
#include "environmentwrapper.h"
#include "basewrapper.h"

using namespace A64FX_COUNTERS;

namespace meric {

class A64FX {

public:
	static void init(bool detailed = true);
	static void close();
	
	static void getCounterValues(std::map<std::string, unsigned long long> & record);
	static double getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, int & cumulative, std::string counter, std::string regionName = "");
	static std::string getSummaryCounterName();
	static std::string getSourceLabel() {return "A64FX";};
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, unsigned long long>& start, std::map<std::string, unsigned long long>& stop, double runtime, std::map <std::string, double> & ratios) {};

	static inline double getUnit(std::string counterName);

	// static void getSamplePeriodInfo(BaseWrapper::SamplePeriodInfo_t &info);

	static bool isAvailable();

private:
	static void initCountersUnits();
#if defined HAVE_LINUX_PERF_EVENT_H && defined __aarch64__
	static long openCounter(perf_event_attr attr, int cpu);
#endif
	static std::map<const std::string, long> countersFd;
	static unsigned long long int COUNTERMAX;
	static std::map<std::string, double> units;
};
}

#endif /* SRC_WRAPPER_A64FXWRAPPER_H_ */
