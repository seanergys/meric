
#ifndef SRC_WRAPPER_RAPLWRAPPER_H_
#define SRC_WRAPPER_RAPLWRAPPER_H_

#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <locale>

#ifdef HAVE_X86_ADAPT_H
	#include <x86_adapt.h>
#elif defined HAVE_MSR_CORE_H
	#include <msr_rapl.h>
#elif defined HAVE_GEOPM_H
	#include <geopm/PlatformTopo.hpp>
	#include <geopm/PlatformIO.hpp>
	#include <geopm_hash.h>
	static geopm::PlatformIO &pio = geopm::platform_io();
#endif

#include "msrs.h"
#include "counters.h"
#include "countersrates.h"
#include "../basis/utils.h"

#ifdef HAVE_X86_ADAPT_H
	using namespace X86ADAPT_RAPL_COUNTERS;
#elif defined HAVE_MSR_CORE_H
	using namespace MSR_RAPL_COUNTERS;
#elif defined HAVE_GEOPM_H
	using namespace GEOPM_RAPL_COUNTERS;
#else
	using namespace X86ADAPT_RAPL_COUNTERS;
#endif


namespace meric {

class RAPL {
#if defined HAVE_X86_ADAPT_H || defined HAVE_MSR_CORE_H || defined HAVE_GEOPM_H
public:
	static void init(bool detailed = false);
	static void close() {};
	
	static void getCounterValues(std::map<std::string, unsigned long long int> & record);
	static void initMap(std::map<std::string, unsigned long long int> & dict);
	static double getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, std::string counter);
/**
 * Since RAPL only provides measurement for specific domains, and we are 
 * interested in the overall energy consumption of the node (domains under one 
 * node) we had to create a "fake" counter
 * getSummaryCounterName() returns name of this counter
 **/
	static inline std::string getSummaryCounterName() { return RAPL_SUM_ENERGY; };
	static std::string getSourceLabel();
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, unsigned long long int>& start, std::map<std::string, unsigned long long int>& stop, double runtime, std::map <std::string, double> & ratios) {};

	static void initCountersUnits();
	static double getUnit(std::string counterName);
private:
		//list of counters
	static std::map<std::string, unsigned long long int> counters;
	static std::string suffix;
	static bool detailedMode;
	static unsigned long long int COUNTERMAX;
#else
public:
	static void init(bool detailed = false) {};
	static void close() {};
	static void getCounterValues(std::map<std::string, unsigned long long int> & record) {};
	static void initMap(std::map<std::string, unsigned long long int> & dict);
	static double getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, std::string counter) {return 0;};
	static std::string summaryCounter();
	static std::string getSummaryCounterName();
	static std::string getSourceLabel();
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, unsigned long long int>& start, std::map<std::string, unsigned long long int>& stop, double runtime, std::map <std::string, double> & ratios) {};
	static void initCountersUnits() {};
	static double getUnit(std::string counterName);
private:
	static std::string suffix;
	static int numOfDevices;
	static bool detailedMode;
	static unsigned long long int COUNTERMAX;
#endif
	static std::map<std::string, double> units;
};

}



#endif /* SRC_WRAPPER_RAPLWRAPPER_H_ */
