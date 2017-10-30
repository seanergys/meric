
#ifndef SRC_WRAPPER_PERFEVENTWRAPPER_H_
#define SRC_WRAPPER_PERFEVENTWRAPPER_H_

#ifdef HAVE_LINUX_PERF_EVENT_H
#include <linux/perf_event.h>
#endif
#include <map>
#include <iostream>
#include <climits>

#include "counters.h"
#include "countersrates.h"

namespace meric {

#ifdef HAVE_LINUX_PERF_EVENT_H

class PERFEVENT {

public:

	static void init();
	static void close();
	
	static void getCounterValues(std::map<std::string, long long> & record);
	static void initMap(std::map<std::string, long long> & dict);
	static unsigned long long getResultValue(long long startValue, long long stopValue, double runtime, std::string counter);
	static void getSummaryValue(double time, unsigned long long int * counterSummary);
	static std::string getSummaryCounterName();
	static std::string getCountersLabel();
	static void getRatios(std::map<std::string, long long>& start, std::map<std::string, long long>& stop, double runtime, std::map <std::string, double> & ratios);
private:
	static void start();
	static long openCounter(perf_event_attr attr);
		//list of counters
	static std::map<std::string,long long> counters;
	static long long COUNTERMAX;
};

#else

class PERFEVENT {
public:
	static void init() {};
	static void close() {};
	static void start() {};
	static void stop() {};
	static void getCounterValues(std::map<std::string, long long> & record) {std::cerr << "PERFEVENT::getCounterValues() TODO\n"; };
	static void initMap(std::map<std::string, long long> & dict);
	static unsigned long long getResultValue(long long startValue, long long stopValue, double runtime, std::string counter);
	static void getSummaryValue(double time, unsigned long long int * counterSummary);
	static std::string getSummaryCounterName();
	static std::string getCountersLabel();
	static void getRatios(std::map<std::string, long long>& start, std::map<std::string, long long>& stop, double runtime, std::map <std::string, double> & ratios);
private:
	static long long COUNTERMAX;
};
#endif

}



#endif /* SRC_WRAPPER_PERFEVENTWRAPPER_H_ */
