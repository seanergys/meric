
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
	
	static void getCounterValues(std::map<std::string, unsigned long long int> & record);
	static unsigned long long getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, std::string counter, std::string regionName);
	static std::string getSummaryCounterName();
	static std::string getSourceLabel();
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, unsigned long long int>& start, std::map<std::string, unsigned long long int>& stop, double runtime, std::map <std::string, double> & ratios);
private:
	static void start();
	static long openCounter(perf_event_attr attr);
	static void initMap(std::map<std::string, unsigned long long int> & dict);
		//list of counters
	static std::map<std::string,unsigned long long int> counters;
	static unsigned long long int COUNTERMAX;
};

#else

class PERFEVENT {
public:
	static void init() {};
	static void close() {};
	static void start() {};
	static void stop() {};
	static void getCounterValues(std::map<std::string, unsigned long long int> & record) {};
	static unsigned long long getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, std::string counter, std::string regionName);
	static std::string getSummaryCounterName();
	static std::string getSourceLabel();
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, unsigned long long int>& start, std::map<std::string, unsigned long long int>& stop, double runtime, std::map <std::string, double> & ratios);
private:
	static void initMap(std::map<std::string, unsigned long long int> & dict);
	static unsigned long long int COUNTERMAX;
};
#endif

}



#endif /* SRC_WRAPPER_PERFEVENTWRAPPER_H_ */
