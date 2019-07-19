/**
 * Tool to provide information about the system parameters that can be tuned.
 **/
#include <unistd.h>
#include <iostream>
#include <math.h>
#include <map>
#include <fstream>
#include <string.h>
#include <sys/stat.h>
#include "../src/wrapper/environmentwrapper.h"

#ifdef HAVE_MSR_CORE_H
	#include <msr_core.h>
	#include <msr_rapl.h>
	#include <msr_clocks.h>
	#include <libmsr_error.h>
#endif

#ifdef HAVE_X86_ADAPT_H
	#include <stdint.h>
	#include <memory>
	#include <x86_adapt.h>
#endif

#ifdef HAVE_NUMA_H
	#include <numa.h>
#endif

/**
 * List of related MSRs with description:
 *
 * MSR_RAPL_POWER_UNIT Register ***********************************************
 * Unit Multipliers used in RAPL Interfaces (R/O)
 * Power Units (bits 3:0):
 	Power related information (in Watts) is based on the multiplier, 1/2^PU; 
 	where PU is an unsigned integer represented by bits 3:0. 
 	Default value is 0011b, indicating power unit is in 1/8 Watts 
increment.
 * Energy Status Units (bits 12:8): 
 	Energy related information (in Joules) is based on the multiplier, 1/2^ESU; 
 	where ESU is an unsigned integer represented by bits 12:8. 
 	Default value is 10000b, indicating energy status unit is 15.3uJ increment.
 * Time Units (bits 19:16): 
	Time related information (in Seconds) is based on the multiplier, 1/2^TU; 
	where TU is an unsigned integer represented by bits 19:16. 
	Default value is 1010b, indicating time unit is in 976us increment.
 *
 * MSR_PKG_POWER_LIMIT register ***********************************************
 * Package Power Limit #1 (bits 14:0): 
	Sets the average power usage limit of the package domain corresponding 
	to time window # 1. The unit of this field is specified by the "Power Units"
	field of MSR_RAPL_POWER_UNIT
 * Enable Power Limit #1 (bit 15): 
	0 = disabled; 1 = enabled.
 * Package Clamping Limitation #1 (bit 16):
 	0 = disabled; 1 = enabled.
	Allow going below OS-requested P/T state setting during time window 
	specified by bits 23:17.
 * Time Window for Power Limit #1 (bits 23:17): 
	Indicates the time window for power limit #1 
	Time limit = 2^Y * (1.0 + Z/4.0) * Time_Unit 
	Here "Y" is the unsigned integer value represented. by bits 21:17, 
	"Z" is an unsigned integer represented by bits 23:22. 
	"Time_Unit" is specified by the "Time Units" field of MSR_RAPL_POWER_UNIT.
 * Reserved (bits 31:24)
 * Package Power Limit #2 (bits 46:32): 
	Sets the average power usage limit of the package domain corre-sponding 
	to time window # 2. The unit of this field is specified by the "Power Units" 
	field of MSR_RAPL_POWER_UNIT.
 * Enable Power Limit #2 (bit 47): 
	0 = disabled; 1 = enabled.
 * Package Clamping Limitation #2 (bit 48):
	0 = disabled; 1 = enabled.
	Allow going below OS-requested P/T state setting during time window 
	specified by bits 23:17.
 * Time Window for Power Limit #2 (bits 55:49): 
	Indicates the time window for power limit #2 
	Time limit = 2^Y * (1.0 + Z/4.0) * Time_Unit 
	Here "Y" is the unsigned integer value represented. by bits 53:49, 
	"Z" is an unsigned integer represented by bits 55:54. 
	"Time_Unit" is specified by the "Time Units" field of MSR_RAPL_POWER_UNIT. 
	This field may have a hard-coded value in hardware and ignores values 
	written by software.
 * Reserved (bits 62:56)
 * Lock (bit 63):
	If set, all write attempts to this MSR are ignored until next 
	RESET.MSR_PKG_ENERGY_STATUS is a read-only MSR. It reports the actual energy 
	use for the package domain. This MSR is updated every ~1msec. It has 
	a wraparound time of around 60 secs when power consumption is high, and may 
	be longer otherwise.
 *
 * MSR_DRAM_POWER_LIMIT register **********************************************
 * allows a software agent to define power limitation for the DRAM domain. Power 
 * limitation is defined in terms of average power usage (Watts) over a time 
 * window specified in MSR_DRAM_POWER_LIMIT. A power limit can be specified 
 * along with a time window. A lock mechanism allow the software agent to 
 * enforce power limit settings. Once the lock bit is set, the power limit 
 * settings are static and unmodifiable until next RESET
 * DRAM Power Limit #1(bits 14:0):
	Sets the average power usage limit of the DRAM domain corresponding to time 
	window # 1. The unit of this field is specified by the "Power Units" field 
	of MSR_RAPL_POWER_UNIT.
 * Enable Power Limit #1(bit 15):
	0 = disabled; 1 = enabled.
 * Clamping limit (bit 16):
	0 = disabled; 1 = enabled.
	Allow going below OS-requested P/T state setting during time window 
	specified by bits 23:17.
 * Time Window for Power Limit (bits 23:17):
	Indicates the length of time window over which the power limit will be used 
	by the processor. The numeric value encoded by bits 23:17 is represented by 
	the product of 2^Y *F; where F is a single-digit decimal floating-point 
	value between 1.0 and 1.3 with the fraction digit represented by bits 23:22, 
	Y is an unsigned integer represented by bits 21:17. The unit of this field 
	is specified by the "Time Units" field of MSR_RAPL_POWER_UNIT.
 * Registred (bits 30:24)
 * Lock (bit 31): 
	If set, all write attempts to this MSR are ignored until next RESET.
 * Registred (bits 63:32)
 *
 * MSR_PKG_POWER_INFO/MSR_DRAM_POWER_INFO register ****************************
 * Read-only MSR. It reports the package/DRAM power range information for RAPL 
 * usage. This MSR provides maximum/minimum values (derived from electrical 
 * specification), thermal specification power of the package/DRAM domain. It 
 * also provides the largest possible time window for software to program the 
 * RAPL interface.
 * Thermal Spec Power (bits 14:0):
	The unsigned integer value is the equivalent of thermal specification power
	of the package/DRAM domain. The unit of this field is specified by the 
	"Power Units" field of MSR_RAPL_POWER_UNIT.
 * Registred (bit 15)
 * Minimum Power (bits 30:16):
	The unsigned integer value is the equivalent of minimum power derived from
	electrical spec of the package/DRAM domain. The unit of this field is 
	specified by the "Power Units" field of MSR_RAPL_POWER_UNIT.
 * Registred (bit 31)
 * Maximum Power (bits 46:32):
	The unsigned integer value is the equivalent of maximum power derived from
	the electrical spec of the package/DRAM domain. The unit of this field is 
	specified by the "Power Units" field of MSR_RAPL_POWER_UNIT.
 * Registred (bit 47)
 * Maximum Time Window (bits 53:48):
	The unsigned integer value is the equivalent of largest acceptable value to 
	program the time window of MSR_PKG_POWER_LIMIT/MSR_DRAM_POWER_LIMIT. The 
	unit of this field is specified by the "Time Units" field of 
	MSR_RAPL_POWER_UNIT.
 * Registred (bits 63:54)
 *
 * MSR_PKG_PERF_STATUS/MSR_DRAM_PERF_STATUS register **************************
 * Read-only MSR. It reports the total time for which the package was throttled 
 * due to the RAPL power limits. Throttling in this context is defined as going 
 * below the OS-requested P-state or T-state. It has a wrap-around time of many 
 * hours. The availability of this MSR is platform specific (see Chapter 2, 
 * "Model-Specific Registers (MSRs)" in the Intel® 64 and IA-32 Architectures 
 * Software Developer's Manual, Volume 4).
 * Accumulated Package Throttled Time (bits 31:0):
	The unsigned integer value represents the cumulative time (since the last 
	time this register is cleared) that the package/DRAM domain has throttled. 
	The unit of this field is specified by the "Time Units" field of 
	MSR_RAPL_POWER_UNIT.
 * Registred (bits 63:32)
 *
 * MSR_UNCORE_RATIO_LIMIT register ********************************************
 * Uncore Ratio Limit (R/W)
 * Out of reset, the min_ratio and max_ratio fields represent the widest 
 * possible range of uncore frequencies. Writing to these fields allows software 
 * to control the minimum and the maximum frequency that hardware will select.
 * MAX_RATIO (bits 6:0):
	This field is used to limit the max ratio of the LLC/Ring.
 * Reserved (bit 7)
 * MIN_RATIO (bits 14:8):
	Writing to this field controls the minimum possible ratio of the LLC/Ring.
 * Reserved (bits 63:15)
 **/

#ifndef IA32_ENERGY_PERF_BIAS
	#define IA32_ENERGY_PERF_BIAS	0x1B0
#endif
#ifndef MSR_UNCORE_RATIO_LIMIT
	#define MSR_UNCORE_RATIO_LIMIT   0x620
#endif
#ifndef MSR_UNCORE_CURRENT_RATIO
	#define MSR_UNCORE_CURRENT_RATIO   0x621
#endif
#ifndef PLATFORM_INFO_ADDR
	#define PLATFORM_INFO_ADDR	0xCE
#endif

const uint64_t MSR_RAPL_POWER_UNIT_power_mask   = 0xF;
const uint64_t MSR_RAPL_POWER_UNIT_energy_mask  = 0x1F00;
const uint64_t MSR_RAPL_POWER_UNIT_energy_shift = 8;
const uint64_t MSR_RAPL_POWER_UNIT_time_mask    = 0xF0000;
const uint64_t MSR_RAPL_POWER_UNIT_time_shift   = 16;
const uint64_t PLATFORM_INFO_ADDR_mask          = 0xFF00;
const uint64_t PLATFORM_INFO_ADDR_shift         = 8;


////////////////////////////////////////////////////////////////////////////////
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
	std::string governor;
	bool restoreFile;
};
std::map<std::string, double> unit;

enum MODE
{
	USING_MSR,
	USING_ADAPT,
	USING_NUMA,
	USING_NONE
};

#ifdef HAVE_MSR_CORE_H

unsigned long discover_nominal_frequency()
{
	uint64_t PLATFORM_INFO_ADDR_bits;
	read_msr_by_idx(0, PLATFORM_INFO_ADDR, &PLATFORM_INFO_ADDR_bits);
	return ((PLATFORM_INFO_ADDR_bits & PLATFORM_INFO_ADDR_mask) >> PLATFORM_INFO_ADDR_shift) * 100000000;
};

#elif defined __x86_64
//#define _GNU_SOURCE
enum reg_cpuid
{
	eax,
	ebx,
	ecx,
	edx
};
 
#define STRING_SIZE 256

inline void CPUID(unsigned int *regs)
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
}

void get_brand_string(char *buff, int buff_size)
{
	unsigned int regs[4];
	unsigned int size_reg = sizeof(unsigned int);
	memset(buff, 0x0, sizeof(char) * buff_size);

	regs[eax] = 0x80000000;
	regs[ebx] = 0x0;
	regs[ecx] = 0x0;
	regs[edx] = 0x0;
	CPUID(regs);

	if(regs[eax] > 0x80000000)
	{
		regs[eax] = 0x80000002;
		regs[ebx] = 0x0;
		regs[ecx] = 0x0;
		regs[edx] = 0x0;
		CPUID(regs);
		memcpy(buff, &regs[eax], size_reg);
		memcpy(buff+(size_reg*1), &regs[ebx], size_reg);
		memcpy(buff+(size_reg*2), &regs[ecx], size_reg);
		memcpy(buff+(size_reg*3), &regs[edx], size_reg);

		regs[eax] = 0x80000003;
		regs[ebx] = 0x0;
		regs[ecx] = 0x0;
		regs[edx] = 0x0;
		CPUID(regs);
		memcpy(buff+(size_reg*4), &regs[eax], size_reg);
		memcpy(buff+(size_reg*5), &regs[ebx], size_reg);
		memcpy(buff+(size_reg*6), &regs[ecx], size_reg);
		memcpy(buff+(size_reg*7), &regs[edx], size_reg);

		regs[eax] = 0x80000004;
		regs[ebx] = 0x0;
		regs[ecx] = 0x0;
		regs[edx] = 0x0;
		CPUID(regs);
		memcpy(buff+(size_reg*8), &regs[eax], size_reg);
		memcpy(buff+(size_reg*9), &regs[ebx], size_reg);
		memcpy(buff+(size_reg*10), &regs[ecx], size_reg);
		memcpy(buff+(size_reg*11), &regs[edx], size_reg);
	}
}

unsigned long discover_nominal_frequency()
{
	unsigned long f;
	char cpu_model[STRING_SIZE];
	const char s1[2] = "@";
	const char s2[4] = "GHz";
	char *token1;
	char *token2;

	get_brand_string(cpu_model, STRING_SIZE);

	token1 = strtok(cpu_model, s1);
	token1 = strtok(NULL, s1);
	token2 = strtok(token1, s2);

	return f = (unsigned long) (atof(token2) * 1E9);
}
#else
unsigned long discover_nominal_frequency() {return 0;};
#endif

void sysInfoSize()
{
    uint64_t cores = 0;
    uint64_t threads = 0;
	uint64_t sockets = 0;
	int Hthreads;
#ifdef HAVE_MSR_CORE_H
	core_config(&cores, &threads, &sockets, &Hthreads);
#elif defined HAVE_X86_ADAPT_H
	sockets = x86_adapt_get_nr_avaible_devices(X86_ADAPT_DIE);
	cores = x86_adapt_get_nr_avaible_devices(X86_ADAPT_CPU)/sockets;
#elif defined HAVE_NUMA_H
	cores = sysconf(_SC_NPROCESSORS_ONLN); //cores per node
	sockets = numa_node_of_cpu(cores-1)+1;
	if (sockets <= 0)
		sockets = 1;
	cores = cores / sockets;
#endif
	std::cout << "# SYSTEM INFORMATION\n"
	          << "\tSockets per Node: " << sockets << std::endl
	          << "\tCores per Socket: " << cores << std::endl;
	if (threads != 0)         
		std::cout << "\tThreads per Core: " << threads << std::endl
		          << "\tHyper-threads:    " << ((Hthreads == 0) ? "false" : "true") << std::endl;
}

bool sysInfoFrequencies(systemLimits& limits)
{
	bool ok = true;
	std::cout << "\n# CPU FREQUENCIES\n";
	std::ifstream frequencyBounderyFile;
	std::string freqBound;

	frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
	frequencyBounderyFile >> freqBound;
	frequencyBounderyFile.close();
	std::cout << "\tCurrent governor:           " << freqBound << std::endl;
	if (freqBound != limits.governor)
		ok = false;
	
	freqBound = "";
	frequencyBounderyFile.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors");
	if (frequencyBounderyFile.is_open())
	{
		getline(frequencyBounderyFile, freqBound);
		frequencyBounderyFile.close();
		std::cout << "\tAvailable governors:        " << freqBound << std::endl;
	}
	
	#ifdef HAVE_X86_ADAPT_H
	bool DVFS = true;
	#else
	bool DVFS = false;
	#endif
	size_t pos;
	std::string token;
	
	if (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", W_OK) == 0)
	{
		while (DVFS == false && (pos = freqBound.find(' ')) != std::string::npos)
		{
			token = freqBound.substr(0, pos);
			if (token == "userspace")
				DVFS = true;
			freqBound.erase(0, pos+1);
		}
	}
	
	if (!DVFS)
	{
		if (access("/sys/devices/system/cpu/intel_pstate/no_turbo", W_OK) != 0 ||
		   (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", W_OK) != 0))
			std::cout << "\tCPU Core Frequency scaling (DVFS) not available\n"
			          << "\t\tadd userspace scaling governor and grand write access to:\n"
			          << "\t\t - /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor\n"
			          << "\t\tor use Intel PState and grand write access to:\n"
			          << "\t\t - /sys/devices/system/cpu/intel_pstate/no_turbo\n"
			          << "\t\t - /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq\n";
	}
	
	long freq = 0;
	//files have the CPU core frequency in KHz
	frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
	frequencyBounderyFile >> freqBound;
	frequencyBounderyFile.close();
	freq = std::stol(freqBound) * 1000;
	std::cout << "\tMax CPU Core Frequency:     " << freq << " Hz\n";
	if (freq != limits.maxCoreFreq)
		ok = false;

	frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
	frequencyBounderyFile >> freqBound;
	frequencyBounderyFile.close();
	freq = std::stol(freqBound) * 1000;
	std::cout << "\tMin CPU Core Frequency:     " << freq << " Hz\n";
	if (freq != limits.minCoreFreq)
		ok = false;

	frequencyBounderyFile.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies");
	if (frequencyBounderyFile.is_open())
	{
		getline(frequencyBounderyFile, freqBound);
		frequencyBounderyFile.close();
		std::cout << "\tAvailable CPU Core Frequencies: " << freqBound << " kHz\n";
	}
	
	unsigned long nominalFreq = discover_nominal_frequency();
	if (nominalFreq != 0)
		std::cout << "\tNominal CPU Core Frequency: " << nominalFreq << " Hz\n";

#ifdef HAVE_MSR_CORE_H
	if (ok)
	{
		uint64_t IA32_ENERGY_PERF_BIAS_bits;
		read_msr_by_idx(0, IA32_ENERGY_PERF_BIAS, &IA32_ENERGY_PERF_BIAS_bits);
		if (IA32_ENERGY_PERF_BIAS_bits != limits.IA32_ENERGY_PERF_BIAS_default)
			ok = false;
	}
#endif

#ifdef HAVE_MSR_CORE_H
	uint64_t MSR_UNCORE_RATIO_LIMIT_bits;
	read_msr_by_idx(0, MSR_UNCORE_RATIO_LIMIT, &MSR_UNCORE_RATIO_LIMIT_bits);
	freq = (MSR_UNCORE_RATIO_LIMIT_bits & MSR_UNCORE_RATIO_LIMIT_max_mask) * 100000000;
	std::cout << "\tMax CPU Uncore Frequency:   " << freq << " Hz\n";
	if (ok && freq != limits.maxUncoreFreq)
		ok = false;
	freq = ((MSR_UNCORE_RATIO_LIMIT_bits & MSR_UNCORE_RATIO_LIMIT_min_mask) >> MSR_UNCORE_RATIO_LIMIT_min_shift) * 100000000;
	std::cout << "\tMin CPU Uncore Frequency:   " << freq << " Hz\n";
	if (ok && freq != limits.minUncoreFreq)
		ok = false;
#elif defined HAVE_X86_ADAPT_H
	int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
	int adaptHandlerUncore = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO");
	uint64_t setting;
	x86_adapt_get_setting(fd, adaptHandlerUncore, &setting);
	freq = setting * 100000000;
	std::cout << "\tMax CPU Uncore Frequency:   " << freq << " Hz\n";
	if (ok && freq != limits.maxUncoreFreq)
		ok = false;
		
	adaptHandlerUncore = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MIN_RATIO");
	x86_adapt_get_setting(fd, adaptHandlerUncore, &setting);
	if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
		std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
	freq = setting * 100000000;
	std::cout << "\tMin CPU Uncore Frequency:   " << freq << " Hz\n";
	if (ok && freq != limits.minUncoreFreq)
		ok = false;
#elif defined __x86_64 //intel CPUs only
	std::cout << "\n# CPU Uncore frequency NOT available - install msr-safe or x86_adapt\n\n";
#endif
	return ok;
}

bool sysInfoPowercap(systemLimits& limits)
{
// /sys/class/powercap/intel-rapl/intel-rapl\:0
	bool ok = true;
	if (unit.size() == 0)
	{
		std::cout << "\n# RAPL power capping NOT available\n\n";
		return ok;
	}

#ifdef HAVE_MSR_CORE_H
// power capping limits ////////////////////////////////////////////////////////
	std::cout << "\n# RAPL POWER LIMITS\n";	
// PKG
	uint64_t MSR_PKG_DRAM_POWER_INFO_bits;
	read_msr_by_idx(0, MSR_PKG_POWER_INFO, &MSR_PKG_DRAM_POWER_INFO_bits);

	std::cout 
		<< "\tRAPL time window unit: " << unit["MSR_RAPL_POWER_UNIT_time"]*1000000 << " us\n"
		<< "\tPKG max power limit:   " 
		<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_power_mask) >> MSR_PKG_DRAM_POWER_INFO_max_power_shift) * unit["MSR_PKG_POWER_LIMIT"] << " W\n"
		
		<< "\tPKG min power limit:   " 
		<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_min_power_mask) >> MSR_PKG_DRAM_POWER_INFO_min_power_shift) * unit["MSR_PKG_POWER_LIMIT"] << " W\n"

//		<< "\tPKG max time window:   " 
//		<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_time_mask) >> MSR_PKG_DRAM_POWER_INFO_max_time_shift) * unit["MSR_RAPL_POWER_UNIT_time"] << " s\n"
		;

// DRAM
	read_msr_by_idx(0, MSR_DRAM_POWER_INFO, &MSR_PKG_DRAM_POWER_INFO_bits);
	std::cout 
		<< "\tDRAM max power limit:  "
		<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_power_mask) >> MSR_PKG_DRAM_POWER_INFO_max_power_shift) * unit["MSR_DRAM_POWER_LIMIT"] << " W\n"

		<< "\tDRAM min power limit:  " 
		<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_min_power_mask) >> MSR_PKG_DRAM_POWER_INFO_min_power_shift) * unit["MSR_DRAM_POWER_LIMIT"] << " W\n"

//		<< "\tDRAM max time window:  " 
//		<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_time_mask) >> MSR_PKG_DRAM_POWER_INFO_max_time_shift) * unit["MSR_RAPL_POWER_UNIT_time"] << " s\n"
		;
#endif


// default power capping ///////////////////////////////////////////////////////
	std::cout << "\n# DEFAULT RAPL POWER LIMITS\n";

// PKG
	uint64_t Y, Z, F;
	uint64_t MSR_PKG_POWER_LIMIT_bits;

#ifdef HAVE_MSR_CORE_H
	read_msr_by_idx(0, MSR_PKG_POWER_LIMIT, &MSR_PKG_POWER_LIMIT_bits);
#elif defined HAVE_X86_ADAPT_H
	int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
	int adaptRegisterHandler;

	adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_1");
	x86_adapt_get_setting(fd, adaptRegisterHandler, &MSR_PKG_POWER_LIMIT_bits);
#endif
	std::cout << "\tPKG power limit #1: " 
		<< (((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_enable_mask) >> MSR_PKG_POWER_LIMIT_enable_shift) == 0 ? "disabled" : "enabled")
		<< " + clamping "
		<< (((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_clamping_mask) >> MSR_PKG_POWER_LIMIT_clamping_shift ) == 0 ? "disabled" : "enabled")
		<< std::endl
		<< "\tPKG power limit #1: " << (MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_mask) * unit["MSR_PKG_POWER_LIMIT"] << " W\n";

	Y = ((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_time_y_mask) >> MSR_PKG_POWER_LIMIT_time_y_shift);
	Z = ((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_time_z_mask) >> MSR_PKG_POWER_LIMIT_time_z_shift);
	unit["MSR_PKG_POWER_LIMIT_timewindow_1"] = pow(2,Y) * (1.0 + Z/4.0);
	std::cout << "\tPKG time window #1: "
		      << unit["MSR_PKG_POWER_LIMIT_timewindow_1"]*unit["MSR_RAPL_POWER_UNIT_time"]
		      << " s\n";

#ifdef HAVE_MSR_CORE_H
	MSR_PKG_POWER_LIMIT_bits = MSR_PKG_POWER_LIMIT_bits >> MSR_PKG_POWER_LIMIT_power2_shift;
#elif defined HAVE_X86_ADAPT_H
	adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_2");
	x86_adapt_get_setting(fd, adaptRegisterHandler, &MSR_PKG_POWER_LIMIT_bits);
#endif

	std::cout << "\tPKG power limit #2: " 
		<< (((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_enable_mask) >> MSR_PKG_POWER_LIMIT_enable_shift) == 0 ? "disabled" : "enabled")
		<< " + clamping "
		<< (((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_clamping_mask) >> MSR_PKG_POWER_LIMIT_clamping_shift) == 0 ? "disabled" : "enabled")
		<< std::endl
		<< "\tPKG power limit #2: " << (MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_mask) * unit["MSR_PKG_POWER_LIMIT"] << " W\n";


	Y = ((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_time_y_mask) >> MSR_PKG_POWER_LIMIT_time_y_shift);
	Z = ((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_time_z_mask) >> MSR_PKG_POWER_LIMIT_time_z_shift);
	unit["MSR_PKG_POWER_LIMIT_timewindow_2"] = pow(2,Y) * (1.0 + Z/4.0);
	std::cout << "\tPKG time window #2: "
		      << unit["MSR_PKG_POWER_LIMIT_timewindow_2"]*unit["MSR_RAPL_POWER_UNIT_time"] 
		      << " s\n";

// DRAM
	uint64_t MSR_DRAM_POWER_LIMIT_bits;
#ifdef HAVE_MSR_CORE_H
	read_msr_by_idx(0, MSR_DRAM_POWER_LIMIT, &MSR_DRAM_POWER_LIMIT_bits);
#elif defined HAVE_X86_ADAPT_H
	adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_RAM_POWER_LIMIT");
	x86_adapt_get_setting(fd, adaptRegisterHandler, &MSR_DRAM_POWER_LIMIT_bits);

	if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
		std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
#endif

	std::cout << "\tDRAM power limit:   " 
		<< (((MSR_DRAM_POWER_LIMIT_bits & MSR_DRAM_POWER_LIMIT_enable_mask) >> MSR_DRAM_POWER_LIMIT_enable_shift) == 0 ? "disabled" : "enabled")
		<< " + clamping "
		<< (((MSR_DRAM_POWER_LIMIT_bits & MSR_DRAM_POWER_LIMIT_clamping_mask) >> MSR_DRAM_POWER_LIMIT_clamping_shift ) == 0 ? "disabled" : "enabled")
		<< std::endl
		<< "\tDRAM power limit:   " << (MSR_DRAM_POWER_LIMIT_bits & MSR_DRAM_POWER_LIMIT_mask) * unit["MSR_DRAM_POWER_LIMIT"] << " W\n";

	F = 1.0 + ((MSR_DRAM_POWER_LIMIT_bits & MSR_DRAM_POWER_LIMIT_time_f_mask) >> MSR_DRAM_POWER_LIMIT_time_f_shift)/10.0;
	Y = ((MSR_DRAM_POWER_LIMIT_bits & MSR_DRAM_POWER_LIMIT_time_y_mask) >> MSR_DRAM_POWER_LIMIT_time_y_shift);
	unit["MSR_DRAM_POWER_LIMIT_timewindow"] = pow(2,Y) * F;
	std::cout << "\tDRAM time window:   "
		      << unit["MSR_DRAM_POWER_LIMIT_timewindow"]*unit["MSR_RAPL_POWER_UNIT_time"]
		      << " s\n";
	return ok;
}

void getRAPLunits()
{
	uint64_t MSR_RAPL_POWER_UNIT_bits = 0x0;
#ifdef HAVE_MSR_CORE_H
	read_msr_by_idx(0, MSR_RAPL_POWER_UNIT, &MSR_RAPL_POWER_UNIT_bits);
#elif defined HAVE_X86_ADAPT_H
	const int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
	if (fd < 0)
	{
		std::cerr << "X86ADAPT RAPL INIT error while accessing file descriptor\n";
		return;
	}
	const int itemId = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_Power_Unit");
	x86_adapt_get_setting(fd, itemId, &MSR_RAPL_POWER_UNIT_bits);
	if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
		std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
#endif
	if (MSR_RAPL_POWER_UNIT_bits != 0)
	{
		unit["MSR_PKG_POWER_LIMIT"] = 1.0/pow(2,(MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_power_mask));
		unit["MSR_DRAM_POWER_LIMIT"] = unit["MSR_PKG_POWER_LIMIT"];
		unit["MSR_PKG_ENERGY_STATUS"] = 1.0/pow(2,((MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_energy_mask) >> MSR_RAPL_POWER_UNIT_energy_shift));
		unit["MSR_DRAM_ENERGY_STATUS"] = 1.0/pow(2,16);
		unit["MSR_RAPL_POWER_UNIT_time"] = 1.0/pow(2,((MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_time_mask) >> MSR_RAPL_POWER_UNIT_time_shift));
	}
}

void sysEnergyMeasurement()
{
	int systems = 0;
	std::cout << "\n# AVAILABLE ENERGY MEASUREMENT SYSTEMS\n";

#if defined HAVE_MSR_CORE_H || defined HAVE_X86_ADAPT_H
	std::cout << "\tRAPL\n";
	systems++;
#endif

#ifdef HAVE_HDEEM_H
	std::cout << "\tHDEEM\n";
	systems++;
#endif

#ifdef HAVE_RESTCLIENT_CPP_RESTCLIENT_H
	std::cout << "\tEXAMON\n";
	systems++;
#endif

	if (systems == 0)
		std::cout << "\tNONE\n";
}

void readRestoreFile(systemLimits& limits)
{
	std::string defaultSettingFile = std::string(MERIC_PATH)+"/build/systemDefault.json";
	struct stat info;
	//read a file that holds the system's default settings
	//we have created this file during a previous MERIC run
	struct json_value_s* root = NULL;
	if(stat(defaultSettingFile.c_str(), &info) == 0)
	{
		limits.restoreFile = true;
		std::ifstream jsonFile (defaultSettingFile);
		std::string line, jsonString = "";
		if (jsonFile.is_open())
		{
			while (getline(jsonFile,line))
			{
				jsonString += " " + line;
			}
			jsonFile.close();
		}
		const char * json = jsonString.c_str();
		struct json_value_s* root = json_parse(json, strlen(json));
		if (root ==  NULL)
		{
			std::cerr << "ERROR: invalid " << defaultSettingFile << std::endl;
			rename(defaultSettingFile.c_str(), (defaultSettingFile+".err").c_str());
			return;
		}
		else
		{
			struct json_object_element_s* data = ((struct json_object_s*)root->payload)->start;

			while(data != NULL)
			{
				if (std::string(data->name->string) == "DVFS")
				{
					struct json_object_element_s* dvfs = ((struct json_object_s*)data->value->payload)->start;
					do
					{
						if (std::string(dvfs->name->string) == "GOVERNOR")
						{
							limits.governor = std::string(((struct json_string_s*)dvfs->value->payload)->string);
						}
						else if (std::string(dvfs->name->string) == "MAX")
						{
							limits.maxCoreFreq = std::stol(std::string(((struct json_string_s*)dvfs->value->payload)->string));
						}
						else if (std::string(dvfs->name->string) == "MIN")
						{
							limits.minCoreFreq = std::stol(std::string(((struct json_string_s*)dvfs->value->payload)->string));
						}
						else if (std::string(dvfs->name->string) == "DEFAULT")
						{
							limits.defaultCoreFreq = std::stol(std::string(((struct json_string_s*)dvfs->value->payload)->string));
						}
						else if (std::string(dvfs->name->string) == "ENERGY_PERF_BIAS")
						{
							limits.IA32_ENERGY_PERF_BIAS_default = std::strtoull(std::string(((struct json_string_s*)dvfs->value->payload)->string).c_str(), NULL, 0);
						}
						dvfs = dvfs->next;
					}while(dvfs != NULL);
				}
				else if (std::string(data->name->string) == "UFS")
				{
					struct json_object_element_s* ufs = ((struct json_object_s*)data->value->payload)->start;
					do
					{
						if (std::string(ufs->name->string) == "MAX")
						{
							limits.maxUncoreFreq = std::stol(std::string(((struct json_string_s*)ufs->value->payload)->string));
						}
						else if (std::string(ufs->name->string) == "MIN")
						{
							limits.minUncoreFreq = std::stol(std::string(((struct json_string_s*)ufs->value->payload)->string));
						}
						ufs = ufs->next;
					}while(ufs != NULL);
				}
				else if (std::string(data->name->string) == "PWRCAP")
				{
					struct json_object_element_s* pwr = ((struct json_object_s*)data->value->payload)->start;
					do
					{
						if (std::string(pwr->name->string) == "PKG")
						{
							limits.defaultPowerCapPKG = std::strtoull(std::string(((struct json_string_s*)pwr->value->payload)->string).c_str(), NULL, 0);
						}
						else if (std::string(pwr->name->string) == "DRAM")
						{
							limits.defaultPowerCapRAM = std::strtoull(std::string(((struct json_string_s*)pwr->value->payload)->string).c_str(), NULL, 0);
						}
						pwr = pwr->next;
					}while(pwr != NULL);
				}
				data = data->next;
			}
			free(root);
		}
	}
	else
		limits.restoreFile = false;
}


//****************************************************************************//
int main(int argc, char **argv)
{
	bool restore = false;
	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg == "-r")
			restore = true;
	}

	MODE status = USING_NONE;
	#ifdef HAVE_MSR_CORE_H
	status = USING_MSR;
	if (init_msr())
	{
		std::cerr << "LIBMSR ERROR: Unable to initialize\n";
	}
	else
	#endif
	{
		#ifdef HAVE_X86_ADAPT_H
		status = USING_ADAPT;
		if (x86_adapt_init())
		{
			std::cerr << "X86ADAPT ERROR: Unable to initialize\n";
		}
		else
		#endif
		{
			#ifdef HAVE_NUMA_H
			status = USING_NUMA;
			#endif
		}
	}
	bool systemInTheDefaultSettings = true;
	systemLimits limits;
	readRestoreFile(limits);
	getRAPLunits();
	sysInfoSize();
	systemInTheDefaultSettings = sysInfoFrequencies(limits);
	systemInTheDefaultSettings = sysInfoPowercap(limits) && systemInTheDefaultSettings;
	sysEnergyMeasurement();
	
	if (!systemInTheDefaultSettings && limits.restoreFile)
	{
		if (restore)
		{
			meric::RegionParameters defaultParameter = { .frequency=0, .uncoreFrequency=0, .nthreads=0, .powerCapPKG=0, .powerCapPKGtime=0};
			meric::Environment::init(meric::SYS_OTHER, defaultParameter, 0);
			meric::Environment::restoreSystemSettings(true);
		}
		else
			std::cerr << "\n\tWARNING: system not in the default settings!"
			          << "\n\trun systemInfo with parameter '-r' to restore the settings\n";
	}

	if (status == USING_MSR)
	{
		#ifdef HAVE_MSR_CORE_H
		finalize_msr();
		#endif
	}
	else if (status == USING_ADAPT)
	{
		#ifdef HAVE_X86_ADAPT_H
		x86_adapt_finalize();
		#endif
	}

	return 0;
}
