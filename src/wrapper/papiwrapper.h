
#ifndef SRC_WRAPPER_PAPIWRAPPER_H_
#define SRC_WRAPPER_PAPIWRAPPER_H_

#include <map>
#include <iostream>
#include <climits>
#include <vector>
#include <omp.h>

#include "counters.h"
#include "countersrates.h"
#include "../store/countersstore.h"


namespace meric {

#ifdef HAVE_PAPI_H

class PAPI{

public:

	static void init();
	static void close();
	
	static void getCounterValues(std::map<std::string, long long> & record);
	static void initMap(std::map<std::string, long long> & dict);
	static unsigned long long getResultValue(long long startValue, long long stopValue, std::string counter);
	static void getSummaryValue(double time, unsigned long long int * counterSummary);
	static std::string getSummaryCounterName();
	static std::string getCountersLabel();
	static void getRatios(CountersRecord start, CountersRecord stop, double runtime, std::map <std::string, double> & ratios);
private:
		//list of counters
	static std::map<std::string, long long> counters;
	static int eventSet;
	static long long * countersValues;
	static long long COUNTERMAX;

};

#else

class PAPI {

public:
	static void init() {};
	static void close() {};
	static void getCounterValues(std::map<std::string, long long> & record) {std::cerr << "PAPI::getCounterValues() TODO\n"; };
	static void initMap(std::map<std::string, long long> & dict);
	static unsigned long long getResultValue(long long startValue, long long stopValue, std::string counter);
	static void getSummaryValue(double time, unsigned long long int * counterSummary);
	static std::string getSummaryCounterName();
	static std::string getCountersLabel();
	static void getRatios(CountersRecord start, CountersRecord stop, double runtime, std::map <std::string, double> & ratios);
private:
	static long long COUNTERMAX;
};


#endif

}



#endif /* SRC_WRAPPER_PAPIWRAPPER_H_ */
