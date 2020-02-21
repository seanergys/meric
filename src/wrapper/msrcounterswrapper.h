#ifndef SRC_WRAPPER_MSRCOUNTERSWRAPPER_H_
#define SRC_WRAPPER_MSRCOUNTERSWRAPPER_H_

#include <map>
#include <iostream>
#include <climits>
#include <vector>
#include <omp.h>

#ifdef HAVE_MSR_CORE_H
	#include "msrs.h"
	#include <msr_clocks.h>
	#include <libmsr_error.h>
#endif

#include "counters.h"
#include "countersrates.h"

namespace meric {

#ifdef HAVE_MSR_CORE_H

class MSR{

public:

	static void init();
	static void close();
	
	static void getCounterValues(std::map<std::string, uint64_t> & record);
	static uint64_t getResultValue(uint64_t startValue, uint64_t stopValue, double runtime, std::string counter);
	static std::string getSummaryCounterName();
	static std::string getCountersLabel();
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, uint64_t>& start, std::map<std::string, uint64_t>& stop, double runtime, std::map <std::string, double> & ratios);
private:
	static void initMap(std::map<std::string,uint64_t> & dict);
		//list of counters
	static std::map<std::string, uint64_t> counters;
	static uint64_t COUNTERMAX;
	static uint64_t nominal_freq;
	static uint64_t CORES_PER_SOCKET;
	static uint64_t SOCKETS_PER_NODE;
};

#else

class MSR {

public:
	static void init() {};
	static void close() {};
	static void getCounterValues(std::map<std::string, uint64_t> & record) {};
	static uint64_t getResultValue(uint64_t startValue, uint64_t stopValue, double runtime, std::string counter);
	static std::string getSummaryCounterName();
	static std::string getCountersLabel();
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, uint64_t>& start, std::map<std::string, uint64_t>& stop, double runtime, std::map <std::string, double> & ratios);
private:
	static void initMap(std::map<std::string, uint64_t> & dict);
	static uint64_t COUNTERMAX;
	static uint64_t nominal_freq;
	static uint64_t CORES_PER_SOCKET;
	static uint64_t SOCKETS_PER_NODE;
};


#endif

}



#endif /* SRC_WRAPPER_MSRCOUNTERSWRAPPER_H_ */
