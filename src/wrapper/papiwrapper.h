
#ifndef SRC_WRAPPER_PAPIWRAPPER_H_
#define SRC_WRAPPER_PAPIWRAPPER_H_

#include <map>
#include <iostream>
#include <climits>
#include <vector>
#include <omp.h>

#include "counters.h"
#include "countersrates.h"


namespace meric {

#ifdef HAVE_PAPI_H

class PAPI{

public:

	static void init();
	static void close();
	
	static void getCounterValues(std::map<std::string, unsigned long long int> & record);
	static void initMap(std::map<std::string, unsigned long long int> & dict);
	static unsigned long long getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, std::string counter);
	static void getSummaryValue(double time, unsigned long long int * counterSummary);
	static std::string getSummaryCounterName();
	static std::string getCountersLabel();
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, unsigned long long int>& start, std::map<std::string, unsigned long long int>& stop, double runtime, std::map <std::string, double> & ratios);
private:
		//list of counters
	static std::map<std::string, unsigned long long int> counters;
	static int eventSet;
	static long long * countersValues;
	static unsigned long long int COUNTERMAX;

};

#else

class PAPI {

public:
	static void init() {};
	static void close() {};
	static void getCounterValues(std::map<std::string, unsigned long long int> & record) {};
	static void initMap(std::map<std::string, unsigned long long int> & dict);
	static unsigned long long getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, std::string counter);
	static void getSummaryValue(double time, unsigned long long int * counterSummary);
	static std::string getSummaryCounterName();
	static std::string getCountersLabel();
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, unsigned long long int>& start, std::map<std::string, unsigned long long int>& stop, double runtime, std::map <std::string, double> & ratios);
private:
	static unsigned long long int COUNTERMAX;
};


#endif

}



#endif /* SRC_WRAPPER_PAPIWRAPPER_H_ */
