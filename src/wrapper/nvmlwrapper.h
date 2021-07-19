
#ifndef SRC_WRAPPER_NVMLWRAPPER_H_
#define SRC_WRAPPER_NVMLWRAPPER_H_

#include "../basis/utils.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <time.h>
#include <ctime>
#include <vector>
#include <map>
#include <cmath>

#ifdef HAVE_NVML_H
	#include <nvml.h>
#endif


namespace meric {

class NVML {

public:
	static void init();
	static void close() {};
	
	static void getCounterValues(std::map<std::string, unsigned long long> & record);
	static void initMap(std::map<std::string, unsigned long long> & dict);
	static double getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, std::string counter, std::string regionName);
	static std::string getSummaryCounterName();
	static std::string getSourceLabel() {return "NVML";};
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, unsigned long long>& start, std::map<std::string, unsigned long long>& stop, double runtime, std::map <std::string, double> & ratios) {};

	static inline double getUnit(std::string counterName) { return 0.001; };
#ifdef HAVE_NVML_H
	static nvmlDevice_t getGPUHandle(int id) { return GPUhandles[id]; };
	static size_t getGPUsPerNode() { return GPUhandles.size(); };
#else
	static size_t getGPUsPerNode() { return 0; };
#endif

private:
		//list of counters
	static std::map<std::string, unsigned long long> counters;
	static unsigned long long COUNTERMAX;

#ifdef HAVE_NVML_H
	static std::vector <nvmlDevice_t> GPUhandles;
#endif
};


}



#endif /* SRC_WRAPPER_NVMLWRAPPER_H_ */
