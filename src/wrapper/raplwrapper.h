
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
	
	static void getCounterValues(std::map<std::string, unsigned long long int> & record);
	static void initMap(std::map<std::string, unsigned long long int> & dict);
	static void detailed() {detailedMode = true;};
	static unsigned long long getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, std::string counter);
	static void getSummaryValue(double time, unsigned long long int * counterSummary);
	static std::string getSummaryCounterName();
	static std::string getCountersLabel();
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, unsigned long long int>& start, std::map<std::string, unsigned long long int>& stop, double runtime, std::map <std::string, double> & ratios) {};
private:
		//list of counters
	static std::map<std::string, unsigned long long int> counters;
	static std::map<std::string, double> energyUnit;
	static std::string suffix;
	static int numOfDevices;
	static bool detailedMode;
	static unsigned long long int COUNTERMAX;
#else
public:
	static const int RAPL_BASELINE = 0;
	static void init() {};
	static void close() {};
	static void getCounterValues(std::map<std::string, unsigned long long int> & record) {};
	static void initMap(std::map<std::string, unsigned long long int> & dict);
	static void detailed() {detailedMode = true;};
	static unsigned long long getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, std::string counter) {return 0;};
	static std::string summaryCounter();
	static void getSummaryValue(double time, unsigned long long int * counterSummary);
	static std::string getSummaryCounterName();
	static std::string getCountersLabel();
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, unsigned long long int>& start, std::map<std::string, unsigned long long int>& stop, double runtime, std::map <std::string, double> & ratios) {};
private:
	static std::string suffix;
	static int numOfDevices;
	static bool detailedMode;
	static unsigned long long int COUNTERMAX;
#endif
};

}



#endif /* SRC_WRAPPER_RAPLWRAPPER_H_ */
