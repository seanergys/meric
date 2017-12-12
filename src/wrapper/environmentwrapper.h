#ifndef SRC_WRAPPER_ENVIRONMENTWRAPPER_H_
#define SRC_WRAPPER_ENVIRONMENTWRAPPER_H_

#include "../meric/meric.h"

#ifdef HAVE_CPUFREQ_H
#include "cpufreq.h"
#endif

#ifdef HAVE_X86_ADAPT_H
#include <stdint.h>
#include <memory>
#include "x86_adapt.h"
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
const int X86ADAPT_MAX_FREQ = 33;
const int CPUFREQ_MAX_FREQ = 25;

static int SOCKET_SIZE;

class Environment {

public:
	static systemLimits FreqLimits [3];
	static int systemType;


	static void frequency(uint64_t freq, int socket = -1);
	static void uncoreFrequency(uint64_t freq, int socket = -1);	
	static void threadNumber(int nthreads);
	static void init(int system, RegionParameters defaultParameter, int rank);
	static void setDefaultFrequencies();
	static void close();

private:

};

}


#endif /* SRC_WRAPPER_ENVIRONMENTWRAPPER_H_ */
