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
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <iterator>
#include <sstream>

namespace meric {

enum mode {MODE_RUN, MODE_TIME, MODE_HDEEM_SAMPLES, MODE_HDEEM_COUNTERS, MODE_RAPL, MODE_JETSON, MODE_THUNDER, MODE_DAVIDE, MODE_NVML};

static std::map <const std::string, mode> modeString = {
	{"NONE", MODE_RUN},
	{"TIME", MODE_TIME},
	{"HDEEM", MODE_HDEEM_SAMPLES}, //default HDEEM
	{"HDEEM-S", MODE_HDEEM_SAMPLES},
	{"HDEEM-C", MODE_HDEEM_COUNTERS},
	{"RAPL", MODE_RAPL},
	{"JETSON", MODE_JETSON},
	{"THUNDER", MODE_THUNDER},
	{"DAVIDE", MODE_DAVIDE},
	{"NVML", MODE_NVML}
	};

enum SYSTEM_TYPE {SYS_HASWELL, SYS_JETSON, SYS_THUNDER, SYS_DAVIDE, SYS_OTHER};

enum DVFS_WAY {NO_DVFS = 0, USRSPACE_FILE, USRSPACE_LIB, INT_PSTATE};

#ifdef __x86_64	// CPUID instruction reading
enum reg_cpuid
{
	eax,
	ebx,
	ecx,
	edx
};
#endif

struct RegionParameters { //must be able to carry -1
	long frequency;       //Hz
	long uncoreFrequency; //Hz
	int  nthreads;        //#omp threads
	long powerCapPKG;     //mW
	long powerCapPKGtime; //us
//	long powerCapRAM;     //mW
//	long powerCapRAMtime; //us
	long frequencyGPU;    //Hz
	long frequencyGPUmem; //Hz
	long powerCapGPU;     //mW
};



struct systemLimits{	//TODO - switch from long to uint64_t - n/a or not supported should be zero
	long maxTurboFreq;           //Hz
	long maxCoreFreq;            //Hz
	long defaultCoreFreq;        //Hz - if the default governor is userspace, we must set this frequency at the end of the run, otherwise zero
	long minCoreFreq;            //Hz

	long maxUncoreFreq;          //Hz
	long defaultUncoreFreq;      //Hz - set only if one specific default frequency exists, otherwise zero
	long minUncoreFreq;          //Hz

	long maxPowerCapPKG;         //mW
	long minPowerCapPKG;         //mW
	long maxPowerCapPKGtime;     //us
	uint64_t defaultPowerCapPKG; //raw register value

	long maxPowerCapRAM;         //mW
	long minPowerCapRAM;         //mW
	long maxPowerCapRAMtime;     //us
	uint64_t defaultPowerCapRAM; //raw register value

	uint64_t IA32_ENERGY_PERF_BIAS_default; //raw register value
	//GPU - NVML uses uint
	uint defaultGPUMemFreq;      //Hz
	uint defaultGPUSMFreq;       //Hz
	uint defaultPowerCapGPU;     //mW
	std::map <uint, std::vector<uint>> availableGPUFreq; //Hz, Hz - MEM: <SM>
};


class Environment {

public:
	static int SOCKETS_PER_NODE;	//TODO private
	static int CORES_PER_SOCKET;
	static int THREADS_PER_CORE;
	
	static std::map<int, std::vector<int>> mapThreadsToCore;
	static std::map<int, std::vector<int>> mapCoresToSocket;

	static SYSTEM_TYPE systemType;

	static void frequency(uint64_t freq, int socket = -1);
	static void uncoreFrequency(uint64_t maxFreq, uint64_t minFreq, int socket = -1);	
	static void threadNumber(int nthreads);
	static void CPUpowercap(uint64_t power, uint64_t timeWindow = 0, int socket = -1);
	static void GPUpowercap(uint power, int GPU = -1);
	static void GPUfrequency(uint freqMEM, uint freqSM, int gpu = -1);

	static void init(SYSTEM_TYPE system, RegionParameters defaultParameter, int rank);
	static void close();

	static void initHWSizes();
	static void initHWSizes(int &sn, int &cs, int &tc, std::vector<int> &online)
	{
		if (!mapThreadsToCore.size())
			initHWSizes();
		sn = SOCKETS_PER_NODE;
		cs = CORES_PER_SOCKET;
		tc = THREADS_PER_CORE;
		
		for (const auto core : mapThreadsToCore)
		{
			for (int thread : core.second)
				online.push_back(thread);	
		}
	};
	static void restoreSystemSettings(bool hardReset = false);
	static systemLimits getSystemSettings() { return defaultSettings; };	//TODO
	static bool readRestoreFile(std::string & restoreJson, timespec & modifTime);
	static long getNominalFrequency();
	static long getMaximumTurboFrequency();
	static std::string getScalingDriver();
	static DVFS_WAY isDVFSpossible();
	static DVFS_WAY activateDVFS(DVFS_WAY way = NO_DVFS);
	
	static void getAvailableCF(std::vector<long> & values);
	static void getAvailableUCF(std::vector<long> & values);
//	static void getAvailablePwrCap(std::vector<long> & power, std::vector<long> & time);
	
#ifdef __x86_64
	static bool isHWPactive(std::string &status);
#endif

private:
	static void readSystemDefaultSettings();
	static void stringOfIDsToVector(std::string list, std::vector<int> &vec, bool lastOnly = false);
#ifdef __x86_64
	static void getCPUbrandString(std::string & CPUname);
	static inline void CPUID(unsigned int *regs)
	{
		asm volatile("cpuid"
			: "=a" (regs[0]), // out EAX
			  "=b" (regs[1]), // out EBX
			  "=c" (regs[2]), // out ECX
			  "=d" (regs[3])  // out EDX
			: "a" (regs[0]),   // in EAX
			  "b" (regs[1]),   // in EBX
			  "c" (regs[2]),   // in ECX
			  "d" (regs[3]));  // in EDX
	};
#endif
#ifdef HAVE_NVML_H
	static void getGPUbrandString(std::string & GPUname);
	static void getAvailableGPUfrequencies(std::map<uint, std::vector<uint>> & values);
	static uint currentGPUMemFreq; //Hz
	static uint currentGPUSMFreq;  //Hz
#endif

	static systemLimits defaultSettings;
	static bool usedDVFS;
	static bool usedUFS;
	static bool usedPWRCAP;
	static bool usedGPUDVFS;
	static bool usedGPUPWRCAP;
	static DVFS_WAY activatedDVFS;
	static bool activatedUFS;
	static bool activatedPWRCAP;
	
	static std::string scalingDriver;
	static std::string scalingGovernor;
};


struct SocketsRegionParameters {
	RegionParameters socket [2];	//TODO remove this constant
/*
	RegionParameters * socket;

	SocketsRegionParameters()
	{
		if (Environment::CORES_PER_SOCKET == 0)
		{
			Environment::initHWSizes();
		}
		socket = (RegionParameters*) malloc (sizeof(RegionParameters) * Environment::CORES_PER_SOCKET);
	}
	
	~SocketsRegionParameters()
	{
		free(socket);
	}
*/
};


}


#endif /* SRC_WRAPPER_ENVIRONMENTWRAPPER_H_ */
