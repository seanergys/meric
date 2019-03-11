/**
 * Tool to provide 
 **/
#include <unistd.h>
#include <iostream>
#include <math.h>
#include <map>
#include <fstream>

#ifdef HAVE_MSR_CORE_H
	#include <msr_core.h>
	#include <msr_rapl.h>
	#include <msr_clocks.h>
	#include <libmsr_error.h>
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

const uint64_t MSR_RAPL_POWER_UNIT_power_mask   = 0xF;
const uint64_t MSR_RAPL_POWER_UNIT_energy_mask  = 0x1F00;
const uint64_t MSR_RAPL_POWER_UNIT_energy_shift = 8;
const uint64_t MSR_RAPL_POWER_UNIT_time_mask    = 0xF0000;
const uint64_t MSR_RAPL_POWER_UNIT_time_shift   = 16;

const uint64_t MSR_DRAM_POWER_LIMIT_mask = 0x7FFF;
const uint64_t MSR_DRAM_POWER_LIMIT_enable_mask  = 0x8000;
const uint64_t MSR_DRAM_POWER_LIMIT_enable_shift = 15;
const uint64_t MSR_DRAM_POWER_LIMIT_clamping_mask = 0x10000;
const uint64_t MSR_DRAM_POWER_LIMIT_clamping_shift = 16;
const uint64_t MSR_DRAM_POWER_LIMIT_time_f_mask  = 0xC00000;
const uint64_t MSR_DRAM_POWER_LIMIT_time_y_mask  = 0x3E0000;
const uint64_t MSR_DRAM_POWER_LIMIT_time_f_shift = 22;
const uint64_t MSR_DRAM_POWER_LIMIT_time_y_shift = 17;

const uint64_t MSR_PKG_POWER_LIMIT_mask = 0x7FFF;
const uint64_t MSR_PKG_POWER_LIMIT_shift = 0;
const uint64_t MSR_PKG_POWER_LIMIT_enable_mask = 0x8000;
const uint64_t MSR_PKG_POWER_LIMIT_enable_shift = 15;
const uint64_t MSR_PKG_POWER_LIMIT_clamping_mask = 0x10000;
const uint64_t MSR_PKG_POWER_LIMIT_clamping_shift = 16;
const uint64_t MSR_PKG_POWER_LIMIT_time_y_mask = 0x3E0000;
const uint64_t MSR_PKG_POWER_LIMIT_time_y_shift = 17;
const uint64_t MSR_PKG_POWER_LIMIT_time_z_mask = 0xC00000;
const uint64_t MSR_PKG_POWER_LIMIT_time_z_shift = 22;

const uint64_t MSR_PKG_POWER_LIMIT_power1_shift = 0;
const uint64_t MSR_PKG_POWER_LIMIT_power2_shift = 32; //use "half register-lenght" shift instead of specifying all the mask and shifts for the second half of the register

const uint64_t MSR_PKG_POWER_LIMIT_lock_mask  = 0x8000000000000000;
const uint64_t MSR_DRAM_POWER_LIMIT_lock_mask = 0x80000000;

const uint64_t MSR_PKG_DRAM_POWER_INFO_min_power_mask = 0x7FFF0000;
const uint64_t MSR_PKG_DRAM_POWER_INFO_min_power_shift = 16;
const uint64_t MSR_PKG_DRAM_POWER_INFO_max_power_mask = 0x7FF700000000;
const uint64_t MSR_PKG_DRAM_POWER_INFO_max_power_shift = 32;
const uint64_t MSR_PKG_DRAM_POWER_INFO_max_time_mask = 0x3F000000000000;
const uint64_t MSR_PKG_DRAM_POWER_INFO_max_time_shift = 48;

const uint64_t MSR_UNCORE_RATIO_LIMIT_max_mask  = 0x7F;
const uint64_t MSR_UNCORE_RATIO_LIMIT_min_mask  = 0x7F00;
const uint64_t MSR_UNCORE_RATIO_LIMIT_min_shift = 8;
////////////////////////////////////////////////////////////////////////////////
std::map<std::string, double> unit;

#include <string.h>

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

void sysInfoSize()
{
    uint64_t cores = 0;
    uint64_t threads = 0;
	uint64_t sockets = 0;
	int Hthreads;
#ifdef HAVE_MSR_CORE_H
	core_config(&cores, &threads, &sockets, &Hthreads);
#endif
	std::cout << "# SYSTEM INFORMATION\n"
	          << "\tSockets per Node: " << sockets << std::endl
	          << "\tCores per Socket: " << cores << std::endl
	          << "\tThreads per Core: " << threads << std::endl
	          << "\tHyper-threads:    " << ((Hthreads == 0) ? "false" : "true") << std::endl;
}

void sysInfoFrequencies()
{
	std::cout << "\n# CPU FREQUENCIES\n";
	std::ifstream frequencyBounderyFile;
	std::string freqBound;

	frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
	frequencyBounderyFile >> freqBound;
	frequencyBounderyFile.close();
	std::cout << "\tCurrent governor:           " << freqBound << std::endl;
	
	freqBound = "";
	frequencyBounderyFile.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors");
	if (frequencyBounderyFile.is_open())
	{
		getline(frequencyBounderyFile, freqBound);
		frequencyBounderyFile.close();
		std::cout << "\tAvailable governors:        " << freqBound << std::endl;
	}
	
	bool DVFS = false;
	size_t pos;
	std::string token;
	while ((pos = freqBound.find(' ')) != std::string::npos)
	{
		token = freqBound.substr(0, pos);
		if (token == "userspace")
		{
			DVFS = true;
			break;
		}
		freqBound.erase(0, pos+1);
	}
	
	if (!DVFS);
	{
		if (access("/sys/devices/system/cpu/intel_pstate/no_turbo", W_OK) != 0 ||
		   (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", W_OK) != 0))
			std::cout << "\tCPU Core Frequency scaling (DVFS) not available\n"
			          << "\t\tadd userspace scaling governor or grand write access to:\n"
			          << "\t\t - /sys/devices/system/cpu/intel_pstate/no_turbo\n"
			          << "\t\t - /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq\n";
	}
	
	//files have the CPU core frequency in KHz
	frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
	frequencyBounderyFile >> freqBound;
	frequencyBounderyFile.close();
	std::cout << "\tMax CPU Core Frequency:     " << std::stol(freqBound) * 1000 << " Hz\n";
	frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
	frequencyBounderyFile >> freqBound;
	frequencyBounderyFile.close();
	std::cout << "\tMin CPU Core Frequency:     " << std::stol(freqBound) * 1000 << " Hz\n";

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

	uint64_t MSR_UNCORE_RATIO_LIMIT_bits;
#ifdef HAVE_MSR_CORE_H
	read_msr_by_idx(0, MSR_UNCORE_RATIO_LIMIT, &MSR_UNCORE_RATIO_LIMIT_bits);
	std::cout << "\tMax CPU Uncore Frequency:   " << (MSR_UNCORE_RATIO_LIMIT_bits & MSR_UNCORE_RATIO_LIMIT_max_mask) * 100000000 << " Hz\n";
	std::cout << "\tMin CPU Uncore Frequency:   " << ((MSR_UNCORE_RATIO_LIMIT_bits & MSR_UNCORE_RATIO_LIMIT_min_mask) >> MSR_UNCORE_RATIO_LIMIT_min_shift) * 100000000 << " Hz\n";
#else
	std::cout << "\n# CPU Uncore frequency NOT available\n\n";
#endif
}

void sysInfoPowercap()
{
#ifndef HAVE_MSR_CORE_H
	std::cout << "\n# RAPL power capping NOT available\n\n";
	return;
#else

// power capping limits
	std::cout << "\n# RAPL POWER LIMITS\n";
// PKG
	uint64_t MSR_PKG_DRAM_POWER_INFO_bits;
	read_msr_by_idx(0, MSR_PKG_POWER_INFO, &MSR_PKG_DRAM_POWER_INFO_bits);
	std::cout << "\tPKG max power limit:  " 
		<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_power_mask) >> MSR_PKG_DRAM_POWER_INFO_max_power_shift) * unit["MSR_PKG_POWER_LIMIT"] << " W\n"
		
		<< "\tPKG min power limit:  " 
		<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_min_power_mask) >> MSR_PKG_DRAM_POWER_INFO_min_power_shift) * unit["MSR_PKG_POWER_LIMIT"] << " W\n"

		<< "\tPKG max time window:  " 
		<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_time_mask) >> MSR_PKG_DRAM_POWER_INFO_max_time_shift) * unit["MSR_RAPL_POWER_UNIT_time"] << " s\n";

// DRAM
	read_msr_by_idx(0, MSR_DRAM_POWER_INFO, &MSR_PKG_DRAM_POWER_INFO_bits);
	std::cout << "\tDRAM max power limit: "
		<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_power_mask) >> MSR_PKG_DRAM_POWER_INFO_max_power_shift) * unit["MSR_DRAM_POWER_LIMIT"] << " W\n"

		<< "\tDRAM min power limit: " 
		<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_min_power_mask) >> MSR_PKG_DRAM_POWER_INFO_min_power_shift) * unit["MSR_DRAM_POWER_LIMIT"] << " W\n"

		<< "\tDRAM min time window: " 
		<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_time_mask) >> MSR_PKG_DRAM_POWER_INFO_max_time_shift) * unit["MSR_RAPL_POWER_UNIT_time"] << " s\n";

// default power capping
	std::cout << "\n# DEFAULT RAPL POWER LIMITS\n";

// PKG
	uint64_t Y, Z, F;
	uint64_t MSR_PKG_POWER_LIMIT_bits;
	read_msr_by_idx(0, MSR_PKG_POWER_LIMIT, &MSR_PKG_POWER_LIMIT_bits);
	std::cout << "\tPKG power limit #1: " 
		<< (((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_enable_mask) >> MSR_PKG_POWER_LIMIT_enable_shift) == 0 ? "disabled" : "enabled")
		<< " + clamping "
		<< (((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_clamping_mask) >> MSR_PKG_POWER_LIMIT_clamping_shift ) == 0 ? "disabled" : "enabled")
		<< std::endl
		<< "\tPKG power limit #1: " << (MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_mask) * unit["MSR_PKG_POWER_LIMIT"] << " W\n";


	std::cout << "\tPKG power limit #2: " 
		<< ((((MSR_PKG_POWER_LIMIT_bits >> MSR_PKG_POWER_LIMIT_power2_shift) & MSR_PKG_POWER_LIMIT_enable_mask) >> MSR_PKG_POWER_LIMIT_enable_shift) == 0 ? "disabled" : "enabled")
		<< " + clamping "
		<< ((((MSR_PKG_POWER_LIMIT_bits >> MSR_PKG_POWER_LIMIT_power2_shift) & MSR_PKG_POWER_LIMIT_clamping_mask) >> MSR_PKG_POWER_LIMIT_clamping_shift) == 0 ? "disabled" : "enabled")
		<< std::endl
		<< "\tPKG power limit #2: " << ((MSR_PKG_POWER_LIMIT_bits >> MSR_PKG_POWER_LIMIT_power2_shift) & MSR_PKG_POWER_LIMIT_mask) * unit["MSR_PKG_POWER_LIMIT"] << " W\n";

	Y = ((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_time_y_mask) >> MSR_PKG_POWER_LIMIT_time_y_shift);
	Z = ((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_time_z_mask) >> MSR_PKG_POWER_LIMIT_time_z_shift);
	unit["MSR_PKG_POWER_LIMIT_timewindow_1"] = pow(2,Y) * (1.0 + Z/4.0);
	std::cout << "\tPKG time window #1: "
		      << unit["MSR_PKG_POWER_LIMIT_timewindow_1"]*unit["MSR_RAPL_POWER_UNIT_time"]
		      << " s\n";

	MSR_PKG_POWER_LIMIT_bits = MSR_PKG_POWER_LIMIT_bits >> MSR_PKG_POWER_LIMIT_power2_shift;
	Y = ((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_time_y_mask) >> MSR_PKG_POWER_LIMIT_time_y_shift);
	Z = ((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_time_z_mask) >> MSR_PKG_POWER_LIMIT_time_z_shift);
	unit["MSR_PKG_POWER_LIMIT_timewindow_2"] = pow(2,Y) * (1.0 + Z/4.0);
	std::cout << "\tPKG time window #2: "
		      << unit["MSR_PKG_POWER_LIMIT_timewindow_2"]*unit["MSR_RAPL_POWER_UNIT_time"] 
		      << " s\n";

// DRAM
	uint64_t MSR_DRAM_POWER_LIMIT_bits;
	read_msr_by_idx(0, MSR_DRAM_POWER_LIMIT, &MSR_DRAM_POWER_LIMIT_bits);

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
#endif
}

void getRAPLunits()
{
#ifdef HAVE_MSR_CORE_H
	uint64_t MSR_RAPL_POWER_UNIT_bits;
	read_msr_by_idx(0, MSR_RAPL_POWER_UNIT, &MSR_RAPL_POWER_UNIT_bits);
	unit["MSR_PKG_POWER_LIMIT"] = 1.0/pow(2,(MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_power_mask));
	unit["MSR_DRAM_POWER_LIMIT"] = unit["MSR_PKG_POWER_LIMIT"];
	unit["MSR_PKG_ENERGY_STATUS"] = 1.0/pow(2,((MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_energy_mask) >> MSR_RAPL_POWER_UNIT_energy_shift));
	unit["MSR_DRAM_ENERGY_STATUS"] = 1.0/pow(2,16);
	unit["MSR_RAPL_POWER_UNIT_time"] = 1.0/pow(2,((MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_time_mask) >> MSR_RAPL_POWER_UNIT_time_shift));
#endif
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

//****************************************************************************//
int main()
{
#ifdef HAVE_MSR_CORE_H
	if (init_msr())
	{
		std::cerr << "LIBMSR ERROR: Unable to initialize\n";
		return 1;
	}
#endif

	getRAPLunits();
	sysInfoSize();
	sysInfoFrequencies();
	sysInfoPowercap();
	sysEnergyMeasurement();

#ifdef HAVE_MSR_CORE_H
	finalize_msr();
#endif
	return 0;
}
