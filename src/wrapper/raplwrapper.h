
#ifndef SRC_WRAPPER_RAPLWRAPPER_H_
#define SRC_WRAPPER_RAPLWRAPPER_H_

#include <map>
#include <vector>
#include <iostream>

#include "counters.h"

namespace meric {

class RAPL {
#ifdef HAVE_X86_ADAPT_H
public:
	static const int RAPL_BASELINE = 0; // ~70 W should be added in the RADAR config file
	static void init();
	static void close() {};
	
	static void getCounterValues(std::map<std::string, long long> & record);
	static void initMap(std::map<std::string, long long> & dict);
	static void detailed() {detailedMode = true;};
	static unsigned long long getResultValue(long long startValue, long long stopValue, std::string counter);
	static unsigned long long getSummaryValue(double time);
	static std::string getSummaryCounterName() {return RAPL_COUNTERS::RAPL_SUM_ENERGY;};
	static std::string getCountersLabel();
private:
		//list of counters
	static std::map<std::string, long long> counters;
	static std::map<std::string, double> energyUnit;
	static std::string suffix;
	static int numOfDevices;
	static bool detailedMode;
	static long long COUNTERMAX;
	static unsigned long long counterSummary;

#else
public:
	static const int RAPL_BASELINE = 0;
	static void init() {};
	static void close() {};
	static void getCounterValues(std::map<std::string, long long> & record) {std::cerr << "RAPL::getCounterValues() TODO\n"; };
	static void initMap(std::map<std::string, long long> & dict);
	static void detailed() {detailedMode = true;};
	static unsigned long long getResultValue(long long startValue, long long stopValue, std::string counter) {return 0;};
	static std::string summaryCounter() {return RAPL_COUNTERS::RAPL_SUM_ENERGY;};
	static unsigned long long getSummaryValue(double time);
	static std::string getCountersLabel();
private:
	static std::string suffix;
	static int numOfDevices;
	static bool detailedMode;
	static long long COUNTERMAX;
#endif
};

}



#endif /* SRC_WRAPPER_RAPLWRAPPER_H_ */
