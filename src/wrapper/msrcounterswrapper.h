#ifndef SRC_WRAPPER_MSRCOUNTERSWRAPPER_H_
#define SRC_WRAPPER_MSRCOUNTERSWRAPPER_H_

#include <map>
#include <iostream>
#include <climits>
#include <vector>
#include <omp.h>

#ifdef HAVE_MSR_CORE_H
	#include <msr_core.h>
	#include <msr_clocks.h>
	#include <libmsr_error.h>

	#ifndef MSR_U_PMON_UCLK_FIXED_CTR
		#define MSR_U_PMON_UCLK_FIXED_CTR 0x704
	#endif
	#ifndef PLATFORM_INFO_ADDR
		#define PLATFORM_INFO_ADDR	0xCE
	#endif

	const uint64_t PLATFORM_INFO_ADDR_mask  = 0xFF00;
	const uint64_t PLATFORM_INFO_ADDR_shift = 8;
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
	static void getSummaryValue(double time, unsigned long long int * counterSummary);
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
};

#else

class MSR {

public:
	static void init() {};
	static void close() {};
	static void getCounterValues(std::map<std::string, uint64_t> & record) {};
	static uint64_t getResultValue(uint64_t startValue, uint64_t stopValue, double runtime, std::string counter);
	static void getSummaryValue(double time, uint64_t * counterSummary);
	static std::string getSummaryCounterName();
	static std::string getCountersLabel();
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, uint64_t>& start, std::map<std::string, uint64_t>& stop, double runtime, std::map <std::string, double> & ratios);
private:
	static void initMap(std::map<std::string, uint64_t> & dict);
	static uint64_t COUNTERMAX;
};


#endif

}



#endif /* SRC_WRAPPER_MSRCOUNTERSWRAPPER_H_ */
