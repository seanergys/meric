#ifndef ROCM_WRAPPER_H_
#define ROCM_WRAPPER_H_

#include "../basis/utils.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <time.h>
#include <ctime>
#include <vector>
#include <map>
#include <cmath>

#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	#include <rocm_smi/rocm_smi.h>
#endif

namespace meric {

class ROCM {

public:
    static void init(bool detailed = true);
    static void close() { /*rsmi_shut_down();*/ };
    
    static void getCounterValues(std::map<std::string, unsigned long long> &record);
    static void initMap(std::map<std::string, unsigned long long> &dict);
    static double getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, int &cumulative, std::string counter, std::string regionName = "");
    static std::string getSummaryCounterName();
    static std::string getSourceLabel() { return "ROCMSMI"; };
    static std::string getCounterLabel(std::string counter);
    static void getRatios(std::map<std::string, unsigned long long>& start, std::map<std::string, unsigned long long>& stop, double runtime, std::map <std::string, double> &ratios) {};

    static inline double getUnit(std::string counterName) { return ROCM::resolution; };

#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
    static size_t getGPUsPerNode() { return GPUindices.size(); };
#else
	static size_t getGPUsPerNode() { return 0; };
#endif



private:
    // Map of counters
    static std::map<std::string, unsigned long long> counters;
    static unsigned long long COUNTERMAX;
	static double resolution;
    static std::vector<uint32_t> GPUindices;
};

}

#endif // ROCM_SMI_WRAPPER_H_

