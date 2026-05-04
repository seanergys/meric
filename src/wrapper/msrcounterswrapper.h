#ifndef SRC_WRAPPER_MSRCOUNTERSWRAPPER_H_
#define SRC_WRAPPER_MSRCOUNTERSWRAPPER_H_

#include <map>
#include <iostream>
#include <climits>
#include <vector>
#include <omp.h>

#include "environmentwrapper.h"
#include "counters.h"
#include "countersrates.h"
#include "hwidsignatures.h"

#ifdef HAVE_MSR_CORE_H
	#include <msr_clocks.h>
	#include <libmsr_error.h>
#endif
#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H
	#include "msrs.h"
#endif

namespace meric {

#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H

class MSR{

public:

	static void init();
	static void close();
	
	static void getCounterValues(std::map<std::string, uint64_t> & record);
	static uint64_t getResultValue(uint64_t startValue, uint64_t stopValue, double runtime, int & cumulative, std::string counter = "", std::string regionName = "");
	static std::string getSummaryCounterName();
	static std::string getSourceLabel();
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, uint64_t>& start, std::map<std::string, uint64_t>& stop, double runtime, std::map <std::string, double> & ratios);
private:
	static void initMap(std::map<std::string,uint64_t> & dict);
		//list of counters
	static std::map<std::string, uint64_t> counters;
	static uint64_t COUNTERMAX;
	static uint64_t nominal_freq;
	static const int TCC = 100; //Thermal Control Circuitry
	static CpuidSignatureValues cpu_arch;
};

#else

class MSR {

public:
	static void init() {};
	static void close() {};
	static void getCounterValues(std::map<std::string, uint64_t> & record) {};
	static uint64_t getResultValue(uint64_t startValue, uint64_t stopValue, double runtime, int & cumulative, std::string counter = "", std::string regionName = "");
	static std::string getSummaryCounterName();
	static std::string getSourceLabel();
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, uint64_t>& start, std::map<std::string, uint64_t>& stop, double runtime, std::map <std::string, double> & ratios);
private:
	static void initMap(std::map<std::string, uint64_t> & dict);
	static uint64_t COUNTERMAX;
	static uint64_t nominal_freq;
	static const int TCC = 100;
	static CpuidSignatureValues cpu_arch;
};


#endif

}



#endif /* SRC_WRAPPER_MSRCOUNTERSWRAPPER_H_ */
