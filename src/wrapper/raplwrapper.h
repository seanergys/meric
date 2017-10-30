
#ifndef SRC_WRAPPER_RAPLWRAPPER_H_
#define SRC_WRAPPER_RAPLWRAPPER_H_

#include <map>
#include <vector>
#include <iostream>
#include <math.h>

#include "counters.h"
#include "countersrates.h"

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
	static unsigned long long getResultValue(long long startValue, long long stopValue, double runtime, std::string counter);
	static void getSummaryValue(double time, unsigned long long int * counterSummary);
	static std::string getSummaryCounterName();
	static std::string getCountersLabel();
	static void getRatios(std::map<std::string, long long>& start, std::map<std::string, long long>& stop, double runtime, std::map <std::string, double> & ratios) {};
private:
		//list of counters
	static std::map<std::string, long long> counters;
	static std::map<std::string, double> energyUnit;
	static std::string suffix;
	static int numOfDevices;
	static bool detailedMode;
	static long long COUNTERMAX;
#else
public:
	static const int RAPL_BASELINE = 0;
	static void init() {};
	static void close() {};
	static void getCounterValues(std::map<std::string, long long> & record) {std::cerr << "RAPL::getCounterValues() TODO\n"; };
	static void initMap(std::map<std::string, long long> & dict);
	static void detailed() {detailedMode = true;};
	static unsigned long long getResultValue(long long startValue, long long stopValue, double runtime, std::string counter) {return 0;};
	static std::string summaryCounter();
	static void getSummaryValue(double time, unsigned long long int * counterSummary);
	static std::string getCountersLabel();
	static void getRatios(std::map<std::string, long long>& start, std::map<std::string, long long>& stop, double runtime, std::map <std::string, double> & ratios) {};
private:
	static std::string suffix;
	static int numOfDevices;
	static bool detailedMode;
	static long long COUNTERMAX;
#endif
};

}



#endif /* SRC_WRAPPER_RAPLWRAPPER_H_ */
