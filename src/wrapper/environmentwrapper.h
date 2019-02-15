#ifndef SRC_WRAPPER_ENVIRONMENTWRAPPER_H_
#define SRC_WRAPPER_ENVIRONMENTWRAPPER_H_

#include "../basis/utils.h"

#ifdef HAVE_CPUFREQ_H
	#include "cpufreq.h"
#endif

#ifdef HAVE_X86_ADAPT_H
	#include <stdint.h>
	#include <memory>
	#include "x86_adapt.h"
#else
	#include <numa.h>
#endif

#ifdef HAVE_MSR_CORE_H
	#include <msr_core.h>
	#include <msr_rapl.h>
	#include <msr_clocks.h>
	#include <libmsr_error.h>

	#ifndef IA32_ENERGY_PERF_BIAS
		#define IA32_ENERGY_PERF_BIAS	0x1B0
	#endif
	#ifndef MSR_UNCORE_RATIO_LIMIT
		#define MSR_UNCORE_RATIO_LIMIT   0x620
	#endif
#endif

#ifdef HAVE_CILK_CILK_API_H
	#include <cilk/cilk_api.h> 
#endif

#ifdef HAVE_OMP_H
	#include <omp.h>
#endif

#include <unistd.h>
#include <iostream>
#include <fstream>

const uint64_t MSR_UNCORE_RATIO_LIMIT_max_mask  = 0x7F;
const uint64_t MSR_UNCORE_RATIO_LIMIT_min_mask  = 0x7F00;
const uint64_t MSR_UNCORE_RATIO_LIMIT_min_shift = 8;

namespace meric {

enum mode {MODE_HDEEM, MODE_RAPL, MODE_BOTH, MODE_RUN, MODE_JETSON, MODE_THUNDER, MODE_DAVIDE, MODE_TIME, MODE_LIMIT}; //keep MODE_LIMIT always as the last MODE in the list
enum system {SYS_HASWELL, SYS_JETSON, SYS_THUNDER, SYS_DAVIDE, SYS_OTHER};

struct RegionParameters {
	long frequency;       //must be able to carry -1
	long uncoreFrequency; //must be able to carry -1
	int nthreads;
};



struct systemLimits{ //Hz
	uint64_t maxCoreFreq;
	uint64_t defaultCoreFreq;
	uint64_t minCoreFreq;
	uint64_t maxUncoreFreq;
	uint64_t defaultUncoreFreq;
	uint64_t minUncoreFreq;
};

const int MSR_FREQ_MULT = 256;

class Environment {

public:
	static int NODE_SIZE_THREAD;
	static int NODE_SIZE_CPU;
	static int NODE_SIZE_SOCKET;
	static int SOCKET_SIZE_CPU;

	static systemLimits FreqLimits [5];
	static int systemType;

	static void frequency(uint64_t freq, int socket = -1);
	static void uncoreFrequency(uint64_t maxFreq, uint64_t minFreq, int socket = -1);	
	static void threadNumber(int nthreads);
	static void init(int system, RegionParameters defaultParameter, int rank);
	static void initHWSizes();
	static void setDefaultFrequencies();
	static void close();

private:
	static uint64_t IA32_ENERGY_PERF_BIAS_default;
	static void getFrequenciesLimits();
	static bool usedDVFS;
	static bool usedUFS;
	static std::string originalGovernor;
	static bool CPUFREQ_GOVERNOR;

};


struct SocketsRegionParameters {
	RegionParameters socket [2];	//TODO remove this constant
/*
	RegionParameters * socket;

	SocketsRegionParameters()
	{
		if (Environment::SOCKET_SIZE_CPU == 0)
		{
			Environment::initHWSizes();
		}
		socket = (RegionParameters*) malloc (sizeof(RegionParameters) * Environment::SOCKET_SIZE_CPU);
	}
	
	~SocketsRegionParameters()
	{
		free(socket);
	}
//*/
};


}


#endif /* SRC_WRAPPER_ENVIRONMENTWRAPPER_H_ */
