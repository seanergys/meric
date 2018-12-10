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

#ifdef HAVE_CILK_CILK_API_H
	#include <cilk/cilk_api.h> 
#endif

#ifdef HAVE_OMP_H
	#include <omp.h>
#endif

#include <unistd.h>
#include <iostream>
#include <fstream>


namespace meric {

enum mode {MODE_HDEEM, MODE_RAPL, MODE_BOTH, MODE_RUN, MODE_JETSON, MODE_THUNDER, MODE_DAVIDE, MODE_TIME, MODE_LIMIT}; //keep MODE_LIMIT always as the last MODE in the list
enum system {SYS_HASWELL, SYS_JETSON, SYS_THUNDER, SYS_DAVIDE, SYS_OTHER};

struct RegionParameters {
	double frequency;
	uint64_t uncoreFrequency;
	int nthreads;
};



struct systemLimits{
	int maxCoreFreq;
	int defaultCoreFreq;
	int minCoreFreq;
	int maxUncoreFreq;
	int defaultUncoreFreq;
	int minUncoreFreq;
};

const int CPUFREQ_FREQ_MULT = 100000;
const int X86ADAPT_FREQ_MULT = 256;
const int CPUFREQ_MAX_FREQ = 25;
const int X86ADAPT_MAX_FREQ = 33;

class Environment {

public:
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
	static void getFrequenciesLimits();
	static bool usedDVFS;
	static bool usedUFS;

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
