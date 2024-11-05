#ifndef SRC_WRAPPER_ROCMWRAPPER_H_
#define SRC_WRAPPER_ROCMWRAPPER_H_

#include "../basis/utils.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <time.h>
#include <ctime>
#include <vector>
#include <map>
#include <cmath>
#include <thread>

#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	#include <rocm_smi/rocm_smi.h>
	#include <hip/hip_version.h>
#endif

namespace meric {

class ROCM {

public:
	static void init(bool detailed = true);
	static void close();

	static void start(unsigned int sampling_frequency);
	static void stop();
	
	static void getCounterValues(std::map<std::string, uint64_t> &record);
	static void initMap(std::map<std::string, uint64_t> &dict);
	static double getResultValue(uint64_t startValue, uint64_t stopValue, double runtime, int &cumulative, std::string counter, std::string regionName = "");
	static std::string getSummaryCounterName();
	static std::string getSourceLabel() { return "ROCMSMI"; };
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, uint64_t>& start, std::map<std::string, uint64_t>& stop, double runtime, std::map <std::string, double> &ratios) {};

	static inline double getUnit(std::string counterName) { return unit; };
#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	static size_t getGPUsPerNode() { return GPUindices.size(); };
#else
	static size_t getGPUsPerNode() { return 0; };
#endif

private:
	// Map of counters
	static std::map<std::string, uint64_t> counters;
	static uint64_t COUNTERMAX;
	static double unit;
	static std::vector<uint32_t> GPUindices;
	static bool HIP_PRIOR_6_2;

	static std::vector <std::vector<double>> samples;
	static std::thread samplingThread;

	static void collectSamples(unsigned int frequency);
};

}

#endif // SRC_WRAPPER_ROCMWRAPPER_H_

