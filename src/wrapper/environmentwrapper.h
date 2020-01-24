#ifndef SRC_WRAPPER_ENVIRONMENTWRAPPER_H_
#define SRC_WRAPPER_ENVIRONMENTWRAPPER_H_

#include "../basis/utils.h"
#include "../basis/json.h"

#ifdef HAVE_CPUFREQ_H
	#include <cpufreq.h>
#endif

#ifdef HAVE_X86_ADAPT_H
	#include <stdint.h>
	#include <memory>
#endif

#ifdef HAVE_MSR_CORE_H
	#include <msr_clocks.h>
	#include <libmsr_error.h>
#endif

#ifdef HAVE_NUMA_H
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
#include <string.h>

namespace meric {

enum mode {MODE_HDEEM, MODE_RAPL, MODE_BOTH, MODE_RUN, MODE_JETSON, MODE_THUNDER, MODE_DAVIDE, MODE_TIME, MODE_LIMIT}; //keep MODE_LIMIT always as the last MODE in the list
enum system {SYS_HASWELL, SYS_JETSON, SYS_THUNDER, SYS_DAVIDE, SYS_OTHER};

struct RegionParameters { //must be able to carry -1
	long frequency;       //Hz
	long uncoreFrequency; //Hz
	int nthreads;         //#omp threads
	long powerCapPKG;     //W
	long powerCapPKGtime; //us
//	long powerCapRAM;     //W
//	long powerCapRAMtime; //us
};



struct systemLimits{
	long maxCoreFreq;            //Hz
	long defaultCoreFreq;        //Hz
	long minCoreFreq;            //Hz
	long maxUncoreFreq;          //Hz
	long defaultUncoreFreq;      //Hz
	long minUncoreFreq;          //Hz

	long maxPowerCapPKG;         //W
	long minPowerCapPKG;         //W
	long maxPowerCapPKGtime;     //us
	uint64_t defaultPowerCapPKG; //raw register value

	long maxPowerCapRAM;         //W
	long minPowerCapRAM;         //W
	long maxPowerCapRAMtime;     //us
	uint64_t defaultPowerCapRAM; //raw register value

	uint64_t IA32_ENERGY_PERF_BIAS_default; //raw register value
};


class Environment {

public:
	static int NODE_SIZE_THREAD;
	static int NODE_SIZE_CPU;
	static int NODE_SIZE_SOCKET;
	static int SOCKET_SIZE_CPU;

	static int systemType;

	static void frequency(uint64_t freq, int socket = -1);
	static void uncoreFrequency(uint64_t maxFreq, uint64_t minFreq, int socket = -1);	
	static void threadNumber(int nthreads);
	static void powercap(uint64_t power, uint64_t timeWindow = 0, int socket = -1);
	static void init(int system, RegionParameters defaultParameter, int rank);
	static void restoreSystemSettings(bool hardReset = false);
	static void close();
	static systemLimits getSystemSettings() { return defaultSettings[systemType]; };

private:
	static void initHWSizes();
	static void getSystemDefaultSettings();
	static void storeSystemDefaultSettings();

	static systemLimits defaultSettings [5];
	static bool usedDVFS;
	static bool usedUFS;
	static bool usedPWRCAP;
	static bool activatedDVFS;
	static bool activatedUFS;
	static bool activatedPWRCAP;
	
	static std::string defaultGovernor;
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
*/
};


}


#endif /* SRC_WRAPPER_ENVIRONMENTWRAPPER_H_ */
