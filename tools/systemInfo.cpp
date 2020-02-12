/**
 * Tool to provide information about the system parameters that can be tuned.
 **/
#include <math.h>
#include <map>
#include <string.h>
#include <sys/stat.h>
#include <thread>
#include "../src/wrapper/environmentwrapper.h"
#include "../src/wrapper/msrs.h"

#ifdef HAVE_MSR_CORE_H
	#include <msr_rapl.h>
#endif

#ifdef HAVE_X86_ADAPT_H
	#include <x86_adapt.h>
#endif

#ifdef HAVE_GEOPM_H
	#include <geopm/PlatformTopo.hpp>
	#include <geopm/PlatformIO.hpp>
	#include <geopm_hash.h>
	static geopm::PlatformIO &pio = geopm::platform_io();
#endif

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

enum MODE	//this application keeps this order
{
	USING_MSR,
	USING_ADAPT,
	USING_GEOPM,
	USING_NUMA,
	USING_NONE
};

#ifdef __x86_64	// CPUID instruction reading
#define STRING_SIZE 256
enum reg_cpuid
{
	eax,
	ebx,
	ecx,
	edx
};

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

bool testHWP()
{
	bool HWP = false;
	unsigned int regs[4];
	unsigned int size_reg = sizeof(unsigned int);

	regs[eax] = 0x6;
	regs[ebx] = 0x0;
	regs[ecx] = 0x0;
	regs[edx] = 0x0;
	CPUID(regs);
	
	if ((regs[eax] & 0x80) >> 7 )
	{
		#ifdef HAVE_MSR_CORE_H
			uint64_t IA32_PM_ENABLE_bits;
			read_msr_by_idx(0, IA32_PM_ENABLE, &IA32_PM_ENABLE_bits);
			HWP = IA32_PM_ENABLE_bits & IA32_PM_ENABLE_mask;
			std::cout << "\tHardware controlled P-State: " << (HWP ? "enabled" : "disabled") << std::endl;
		//#else
		//	std::cout << "\tHardware controlled P-State: might be available\n";
		#endif
	}
	else
		std::cout << "\tHardware controlled P-State: not available\n";

	return HWP;
}

#endif

#ifdef HAVE_MSR_CORE_H
unsigned long discover_nominal_frequency()
{
	uint64_t MSR_PLATFORM_INFO_bits;
	read_msr_by_idx(0, MSR_PLATFORM_INFO, &MSR_PLATFORM_INFO_bits);
	return ((MSR_PLATFORM_INFO_bits & MSR_PLATFORM_INFO_nominal_freq_mask) >> MSR_PLATFORM_INFO_nominal_freq_shift) * 100000000;
};

#elif defined __x86_64
//#define _GNU_SOURCE

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
	int Hthreads = 0;
#ifdef HAVE_MSR_CORE_H
	core_config(&cores, &threads, &sockets, &Hthreads);
#else

	#if defined HAVE_X86_ADAPT_H
		sockets = x86_adapt_get_nr_avaible_devices(X86_ADAPT_DIE);
		cores = x86_adapt_get_nr_avaible_devices(X86_ADAPT_CPU)/sockets;
	#elif defined HAVE_GEOPM_H
		const geopm::PlatformTopo::PlatformTopo &topo = geopm::platform_topo();
		cores = topo.num_domain(GEOPM_DOMAIN_CORE);
		sockets = topo.num_domain(GEOPM_DOMAIN_PACKAGE);
	//	NODE_SIZE_THREAD = topo.num_domain(GEOPM_DOMAIN_CPU) * NODE_SIZE_CPU;
		cores = cores / sockets;
	#elif defined HAVE_NUMA_H
		cores = sysconf(_SC_NPROCESSORS_ONLN); //cores per node
		sockets = numa_node_of_cpu(cores-1)+1;
		if (sockets <= 0)
			sockets = 1;
		cores = cores / sockets;
	#endif

	std::string topoStr;
	std::ifstream topoFile("/sys/devices/system/cpu/cpu0/topology/thread_siblings_list");
	topoFile >> topoStr;
	topoFile.close();
	
	while(topoStr.size())
	{
		std::string num = topoStr;
		size_t pos = std::string::npos;
		if ((pos = topoStr.find(',')) != std::string::npos)
		{
			num = topoStr.substr(0,pos);
			topoStr = topoStr.substr(pos+1);
		}
		else
			topoStr = "";
		
		if ((pos = num.find('-')) != std::string::npos)
		{
			long start = std::stol(num.substr(0,pos));
			long stop  = std::stol(num.substr(pos+1));
			threads += stop - start +1;
		}
		else
			threads++;
	}

	if (threads > 1)
	{
		Hthreads = 1;
		cores = cores/threads;
	}
#endif
    
	std::cout << "# SYSTEM INFORMATION\n"
	          << "\tSockets per Node: " << sockets << std::endl
	          << "\tCores per Socket: " << cores << std::endl
	          << "\tThreads per Core: " << threads << std::endl
	          << "\tHyper-threads:    " << ((Hthreads == 0) ? "false" : "true") << std::endl;
}

bool sysInfoFrequencies(systemLimits& limits)
{
	bool DVFS = false;
	bool ok = true;
// CORE FREQUENCY
	std::cout << "\n# CPU FREQUENCIES\n";
	std::ifstream frequencyBounderyFile;
	std::string freqBound = "";
	std::string scalingDriver = "";

	frequencyBounderyFile.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_driver");
	if (frequencyBounderyFile.is_open())
	{
		frequencyBounderyFile >> scalingDriver;
		frequencyBounderyFile.close();
		std::cout << "\tCurrent scaling driver:      " << scalingDriver << std::endl;
	}
	
	frequencyBounderyFile.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
	if (frequencyBounderyFile.is_open())
	{
		freqBound = "";
		frequencyBounderyFile >> freqBound;
		frequencyBounderyFile.close();
		std::cout << "\tCurrent scaling governor:    " << freqBound << std::endl;
		if (freqBound != limits.governor)
			ok = false;
		if (freqBound == "userspace")
		{
#if !defined(HAVE_MSR_CORE_H) && !defined(HAVE_X86_ADAPT_H) && !defined(HAVE_CPUFREQ_H)
			if (access("/sys/devices/system/cpu/cpu*/cpufreq/scaling_setspeed", W_OK) == 0)
#endif
				DVFS = true;
		}
	}
	
	frequencyBounderyFile.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors");
	if (frequencyBounderyFile.is_open())
	{
		freqBound = "";
		getline(frequencyBounderyFile, freqBound);
		frequencyBounderyFile.close();
		std::cout << "\tAvailable governors:         " << freqBound << std::endl;
	}

	if (!DVFS)
	{
		if (scalingDriver == "acpi-cpufreq")
		{
			if (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", W_OK) == 0)
			{
#if !defined(HAVE_MSR_CORE_H) && !defined(HAVE_X86_ADAPT_H) && !defined(HAVE_CPUFREQ_H)
				if (access("/sys/devices/system/cpu/cpu*/cpufreq/scaling_setspeed", W_OK) == 0)
#endif
					DVFS = true;
			}
		}
#if defined HAVE_MSR_CORE_H && defined HAVE_X86_ADAPT_H && defined HAVE_CPUFREQ_H
		else if (scalingDriver == "intel_pstate")
		{
			if (access("/sys/devices/system/cpu/intel_pstate/no_turbo", W_OK) == 0 &&
			   (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", W_OK) == 0))
			   DVFS = true;
		}
#endif
	}

	bool HWP = false;
	#ifdef __x86_64
		HWP = testHWP();
	#endif
	
	if (!DVFS || HWP)
	{
		std::cout << "\n\tCPU core frequency scaling (DVFS) NOT available\n";
		if (HWP)
			std::cout << "\t\tBecause Hardware Controlled P-Stated is activated in the BIOS\n";
		else
		{
#if !defined(HAVE_MSR_CORE_H) && !defined(HAVE_X86_ADAPT_H) && !defined(HAVE_CPUFREQ_H)
			std::cout << "\t\t install one the following libraries:\n"
			          << "\t\t - libmsr / x86_adapt / cpufreq\n";
#endif
			if (scalingDriver == "intel_pstate")
			{
				std::cout << "\t\t grant write access to:\n"
					      << "\t\t - /sys/devices/system/cpu/intel_pstate/no_turbo\n"
					      << "\t\t - /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq\n"
					      << "\t\t or switch the scaling driver to 'acpi-cpufreq' and\n";
			}
			std::cout << "\t\t grant write access to:\n"
				      << "\t\t - /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor\n";			          
#if !defined(HAVE_MSR_CORE_H) && !defined(HAVE_X86_ADAPT_H) && !defined(HAVE_CPUFREQ_H)
			std::cout << "\t\t - /sys/devices/system/cpu/cpu*/cpufreq/scaling_setspeed\n";
#endif
		}
		std::cout << std::endl;
	}
	
	long freq = 0;
	//files have the CPU core frequency in KHz
	frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
	if (frequencyBounderyFile.is_open())
	{
		frequencyBounderyFile >> freqBound;
		frequencyBounderyFile.close();
		freq = std::stol(freqBound) * 1000;
		std::cout << "\tMax CPU core frequency:     " << freq << " Hz\n";
		if (freq != limits.maxCoreFreq)
			ok = false;
	}
	
	frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
	if (frequencyBounderyFile.is_open())
	{
		frequencyBounderyFile >> freqBound;
		frequencyBounderyFile.close();
		freq = std::stol(freqBound) * 1000;
		std::cout << "\tMin CPU core frequency:     " << freq << " Hz\n";
		if (freq != limits.minCoreFreq)
			ok = false;
	}
	
	frequencyBounderyFile.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies");
	if (frequencyBounderyFile.is_open())
	{
		getline(frequencyBounderyFile, freqBound);
		frequencyBounderyFile.close();
		std::cout << "\tAvailable CPU core frequencies: " << freqBound << " kHz\n";
	}
	
	unsigned long nominalFreq = discover_nominal_frequency();
	if (nominalFreq != 0)
		std::cout << "\tNominal CPU core frequency: " << nominalFreq << " Hz\n";

	freq = -1;
	if (scalingDriver == "acpi-cpufreq")
	{
		frequencyBounderyFile.open("/sys/devices/system/cpu/cpufreq/boost");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
			freq = std::stoi(freqBound);
		}
	}
	else if (scalingDriver == "intel_pstate")
	{
		frequencyBounderyFile.open("/sys/devices/system/cpu/intel_pstate/no_turbo");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
			freq = std::stoi(freqBound) == 0 ? 1 : 0;
		}
	}
	if (freq == 0)
	{
		std::cout << "\tTurbo CPU core frequencies: disabled\n";
	}
	else if (freq == 1)
	{
		std::cout << "\tTurbo CPU core frequencies: ";
#ifdef HAVE_MSR_CORE_H
		uint64_t MSR_TURBO_RATIO_LIMIT_bits = 0x0;
		read_msr_by_idx(0, MSR_TURBO_RATIO_LIMIT, &MSR_TURBO_RATIO_LIMIT_bits);

		uint64_t MSR_TURBO_RATIO_LIMIT_CORES_bits = 0x0;
		read_msr_by_idx(0, MSR_TURBO_RATIO_LIMIT_CORES, &MSR_TURBO_RATIO_LIMIT_CORES_bits);
		
		std::vector <int> turboRatio;
		std::vector <int> turboCoresRange;
		for (int i=0; i<64; i+=MSR_TURBO_RATIO_LIMIT_shift)
		{
			turboRatio.push_back(MSR_TURBO_RATIO_LIMIT_CORES_bits && MSR_TURBO_RATIO_LIMIT_mask);
			MSR_TURBO_RATIO_LIMIT_CORES_bits = MSR_TURBO_RATIO_LIMIT_CORES_bits << MSR_TURBO_RATIO_LIMIT_shift;
		}

		if (turboRatio[0] < turboRatio[1]) //the register is MSR_TURBO_RATIO_LIMIT_CORES
		{
			turboCoresRange = turboRatio;
			turboRatio.clear();
		}
		else //the register is MSR_TURBO_RATIO_LIMIT1
		{
			turboCoresRange = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
		}
		
		std::vector<int>::iterator it = turboRatio.begin();
		for (int i=0; i<64; i+=MSR_TURBO_RATIO_LIMIT_shift)
		{
			turboRatio.insert(it++;MSR_TURBO_RATIO_LIMIT_bits && MSR_TURBO_RATIO_LIMIT_mask)
			MSR_TURBO_RATIO_LIMIT_bits = MSR_TURBO_RATIO_LIMIT_bits << MSR_TURBO_RATIO_LIMIT_shift;
		}
		int i = 0;
		while (i < turboRatio.size() && turboRatio[i] != 0)
		{
			std::cout << turboRatio[i] << "(" << turboCoresRange[i]<<") ";
		}
		std::cout << std::endl;
#else
		std::cout << "enabled\n";
#endif		
	}

// IA32_ENERGY_PERF_BIAS
#ifdef HAVE_MSR_CORE_H
	if (ok)
	{
		uint64_t IA32_ENERGY_PERF_BIAS_bits = 0x0;
		read_msr_by_idx(0, IA32_ENERGY_PERF_BIAS, &IA32_ENERGY_PERF_BIAS_bits);
		if (IA32_ENERGY_PERF_BIAS_bits != limits.IA32_ENERGY_PERF_BIAS_default)
			ok = false;
	}
#endif

// UNCORE FREQUENCY
#ifdef HAVE_MSR_CORE_H
	uint64_t MSR_UNCORE_RATIO_LIMIT_bits = 0x0;
	read_msr_by_idx(0, MSR_UNCORE_RATIO_LIMIT, &MSR_UNCORE_RATIO_LIMIT_bits);
	freq = (MSR_UNCORE_RATIO_LIMIT_bits & MSR_UNCORE_RATIO_LIMIT_max_mask) * 100000000;
	std::cout << "\tMax CPU uncore frequency:   " << freq << " Hz\n";
	if (ok && freq != limits.maxUncoreFreq)
		ok = false;
	freq = ((MSR_UNCORE_RATIO_LIMIT_bits & MSR_UNCORE_RATIO_LIMIT_min_mask) >> MSR_UNCORE_RATIO_LIMIT_min_shift) * 100000000;
	std::cout << "\tMin CPU uncore frequency:   " << freq << " Hz\n";
	if (ok && freq != limits.minUncoreFreq)
		ok = false;
#elif defined HAVE_X86_ADAPT_H
	int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
	int adaptHandlerUncore = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO");
	uint64_t setting = 0;
	x86_adapt_get_setting(fd, adaptHandlerUncore, &setting);
	freq = setting * 100000000;
	std::cout << "\tMax CPU uncore frequency:   " << freq << " Hz\n";
	if (ok && freq != limits.maxUncoreFreq)
		ok = false;
		
	adaptHandlerUncore = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MIN_RATIO");
	x86_adapt_get_setting(fd, adaptHandlerUncore, &setting);
	if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
		std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
	freq = setting * 100000000;
	std::cout << "\tMin CPU uncore frequency:   " << freq << " Hz\n";
	if (ok && freq != limits.minUncoreFreq)
		ok = false;
#elif defined HAVE_GEOPM_H
	freq = pio.read_signal("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", pio.signal_domain_type("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO"), 0);
	std::cout << "\tMax CPU uncore frequency:   " << freq << " Hz\n";
	if (ok && freq != limits.maxUncoreFreq)
		ok = false;
	
	freq = pio.read_signal("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", pio.signal_domain_type("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO"), 0);
	std::cout << "\tMin CPU uncore frequency:   " << freq << " Hz\n";
	if (ok && freq != limits.minUncoreFreq)
		ok = false;	
#elif defined __x86_64 //intel CPUs only
	std::cout << "\n# CPU uncore frequency NOT available - install libmsr or x86_adapt\n";
#endif
	return ok;
}

bool sysInfoPowercap(systemLimits& limits)
{
	bool ok = true;
	std::ifstream pwrCapFile;
	std::string pwrCapInfo = "";

	pwrCapFile.open("/sys/class/powercap/intel-rapl/intel-rapl:0/enabled");
	if (pwrCapFile.is_open())
	{
		pwrCapFile >> pwrCapInfo;
		pwrCapFile.close();
	}

	if (pwrCapInfo == "")
	{
		std::cout << "\n# RAPL power capping NOT available\n\n";
		return ok;
	}

// power capping limits ////////////////////////////////////////////////////////
	std::cout << "\n# RAPL POWER LIMITS\n";	
#ifdef HAVE_MSR_CORE_H
// PKG
	uint64_t MSR_PKG_DRAM_POWER_INFO_bits = 0x0;
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
#else
	pwrCapFile.open("/sys/class/powercap/intel-rapl/intel-rapl:0/constraint_0_max_power_uw");
	if (pwrCapFile.is_open())
	{
		pwrCapInfo = "";
		pwrCapFile >> pwrCapInfo;
		pwrCapFile.close();
		std::cout << "\tPKG max power limit #1: " << ((pwrCapInfo == "" || pwrCapInfo == "0") ? "n/a" : std::to_string(std::stol(pwrCapInfo)/1000000)+" W") << "\n";
	}

	pwrCapFile.open("/sys/class/powercap/intel-rapl/intel-rapl:0/constraint_1_max_power_uw");
	if (pwrCapFile.is_open())
	{
		pwrCapInfo = "";
		pwrCapFile >> pwrCapInfo;
		pwrCapFile.close();
		std::cout << "\tPKG max power limit #2: " << ((pwrCapInfo == "" || pwrCapInfo == "0") ? "n/a" : std::to_string(std::stol(pwrCapInfo)/1000000)+" W") << "\n";
	}

	pwrCapFile.open("/sys/class/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:0/constraint_0_max_power_uw");
	if (pwrCapFile.is_open())
	{
		pwrCapInfo = "";
		pwrCapFile >> pwrCapInfo;
		pwrCapFile.close();
		std::cout << "\tDRAM max power limit:   " << ((pwrCapInfo == "" || pwrCapInfo == "0") ? "n/a" : std::to_string(std::stol(pwrCapInfo)/1000000)+" W") << "\n";
	}
#endif

#if defined __x86_64 && !defined HAVE_MSR_CORE_H && ! defined HAVE_X86_ADAPT_H //currently supported only intel
	std::cout << "\tchanging RAPL power capping limits NOT available - install libmsr or x86_adapt\n";
#endif

// default power capping ///////////////////////////////////////////////////////
	std::cout << "\n# DEFAULT RAPL POWER LIMITS\n";
	uint64_t X, Y, Z, F;

#if defined HAVE_MSR_CORE_H || defined HAVE_X86_ADAPT_H || defined HAVE_GEOPM_H
	// PKG
		uint64_t MSR_PKG_POWER_LIMIT_bits = 0x0;
	#ifdef HAVE_MSR_CORE_H
		read_msr_by_idx(0, MSR_PKG_POWER_LIMIT, &MSR_PKG_POWER_LIMIT_bits);
	#elif defined HAVE_X86_ADAPT_H
		int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
		int adaptRegisterHandler;

		adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_1");
		x86_adapt_get_setting(fd, adaptRegisterHandler, &MSR_PKG_POWER_LIMIT_bits);
	#elif defined HAVE_GEOPM_H
		MSR_PKG_POWER_LIMIT_bits = geopm_signal_to_field(pio.read_signal("MSR::PKG_POWER_LIMIT#", pio.signal_domain_type("MSR::PKG_POWER_LIMIT#"), 0)) & 0xFFFFFFFF;
	#endif
		if (MSR_PKG_POWER_LIMIT_bits != 0x0)
		{
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
	#elif defined HAVE_GEOPM_H
				MSR_PKG_POWER_LIMIT_bits = geopm_signal_to_field(pio.read_signal("MSR::PKG_POWER_LIMIT#", pio.signal_domain_type("MSR::PKG_POWER_LIMIT#"), 0)) >> MSR_PKG_POWER_LIMIT_power2_shift;
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
		}

	// DRAM
		uint64_t MSR_DRAM_POWER_LIMIT_bits = 0x0;
	#ifdef HAVE_MSR_CORE_H
		read_msr_by_idx(0, MSR_DRAM_POWER_LIMIT, &MSR_DRAM_POWER_LIMIT_bits);
	#elif defined HAVE_X86_ADAPT_H
		adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_RAM_POWER_LIMIT");
		x86_adapt_get_setting(fd, adaptRegisterHandler, &MSR_DRAM_POWER_LIMIT_bits);

		if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
			std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
	#elif defined HAVE_GEOPM_H
		MSR_DRAM_POWER_LIMIT_bits = geopm_signal_to_field(pio.read_signal("MSR::DRAM_POWER_LIMIT#", pio.signal_domain_type("MSR::DRAM_POWER_LIMIT#"), 0));
	#endif
		if (MSR_DRAM_POWER_LIMIT_bits != 0x0)
		{
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
		}
#else
	// PKG
	pwrCapFile.open("/sys/class/powercap/intel-rapl/intel-rapl:0/enabled");
	if (pwrCapFile.is_open())
	{
		pwrCapInfo = "";
		pwrCapFile >> pwrCapInfo;
		pwrCapFile.close();
		std::cout << "\tPKG power capping:  " << (pwrCapInfo == "0" ? "disabled" : "enabled") << std::endl;
	}

	pwrCapFile.open("/sys/class/powercap/intel-rapl/intel-rapl:0/constraint_0_power_limit_uw");
	if (pwrCapFile.is_open())
	{
		pwrCapInfo = "";
		pwrCapFile >> pwrCapInfo;
		pwrCapFile.close();
		if (pwrCapInfo != "")
			std::cout << "\tPKG power limit #1: " << std::stol(pwrCapInfo)/1000000 << " W\n";
	}
	
	pwrCapFile.open("/sys/class/powercap/intel-rapl/intel-rapl:0/constraint_0_time_window_us");
	if (pwrCapFile.is_open())
	{
		pwrCapInfo = "";
		pwrCapFile >> pwrCapInfo;
		pwrCapFile.close();
		if (pwrCapInfo != "")
			std::cout << "\tPKG time window #1: " << std::stol(pwrCapInfo)/1000000.0 << " s\n";
	}
	
	pwrCapFile.open("/sys/class/powercap/intel-rapl/intel-rapl:0/constraint_1_power_limit_uw");
	if (pwrCapFile.is_open())
	{
		pwrCapInfo = "";
		pwrCapFile >> pwrCapInfo;
		pwrCapFile.close();
		if (pwrCapInfo != "")
			std::cout << "\tPKG power limit #2: " << std::stol(pwrCapInfo)/1000000 << " W\n";
	}

	pwrCapFile.open("/sys/class/powercap/intel-rapl/intel-rapl:0/constraint_1_time_window_us");
	if (pwrCapFile.is_open())
	{
		pwrCapInfo = "";
		pwrCapFile >> pwrCapInfo;
		pwrCapFile.close();
		if (pwrCapInfo != "")
			std::cout << "\tPKG time window #2: " << std::stol(pwrCapInfo)/1000000.0 << " s\n";
	}

	// DRAM
	pwrCapFile.open("/sys/class/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:0/enabled");
	if (pwrCapFile.is_open())
	{
		pwrCapInfo = "";
		pwrCapFile >> pwrCapInfo;
		pwrCapFile.close();
		std::cout << "\tDRAM power capping: " << (pwrCapInfo == "0" ? "disabled" : "enabled") << std::endl;
	}
	
	pwrCapFile.open("/sys/class/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:0/constraint_0_power_limit_uw");
	if (pwrCapFile.is_open())
	{
		pwrCapInfo = "";
		pwrCapFile >> pwrCapInfo;
		pwrCapFile.close();
		if (pwrCapInfo != "")
			std::cout << "\tDRAM power limit:   " << std::stol(pwrCapInfo)/1000000 << " W\n";
	}

	pwrCapFile.open("/sys/class/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:0/constraint_0_time_window_us");
	if (pwrCapFile.is_open())
	{
		pwrCapInfo = "";
		pwrCapFile >> pwrCapInfo;
		pwrCapFile.close();
		if (pwrCapInfo != "")
			std::cout << "\tDRAM time window:   " << std::stol(pwrCapInfo)/1000000.0 << " s\n";
	}
#endif

// PLATFORM
/*
	uint64_t MSR_PLATFORM_POWER_LIMIT_bits = 0x0;
#ifdef HAVE_MSR_CORE_H
	read_msr_by_idx(0, MSR_PLATFORM_POWER_LIMIT, &MSR_PLATFORM_POWER_LIMIT_bits);
#endif
	if (MSR_PLATFORM_POWER_LIMIT_bits != 0x0)
	{
		std::cout << "\tPLATFORM power limit #1: " 
			<< (((MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_enable_mask) >> MSR_PLATFORM_POWER_LIMIT_enable_shift) == 0 ? "disabled" : "enabled")
			<< " + clamping "
			<< (((MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_clamping_mask) >> MSR_PLATFORM_POWER_LIMIT_clamping_shift ) == 0 ? "disabled" : "enabled")
			<< std::endl
			<< "\tPLATFORM power limit #1: " << (MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_power_mask) * unit["MSR_PLATFORM_POWER_LIMIT"] << " W\n";

		Y = ((MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_time_y_mask) >> MSR_PLATFORM_POWER_LIMIT_time_y_shift);
		X = ((MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_time_x_mask) >> MSR_PLATFORM_POWER_LIMIT_time_x_shift);
		unit["MSR_PLATFORM_POWER_LIMIT_timewindow"] = pow(2,Y) * (1.0 + X/4.0);
		std::cout << "\tPLATFORM time window: "
				  << unit["MSR_PLATFORM_POWER_LIMIT_timewindow"]*unit["MSR_RAPL_POWER_UNIT_time"]
				  << " s\n";
		const bool lock = ((MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_lock_mask) >> MSR_PLATFORM_POWER_LIMIT_lock_shift);

		MSR_PLATFORM_POWER_LIMIT_bits = MSR_PLATFORM_POWER_LIMIT_bits >> MSR_PLATFORM_POWER_LIMIT_power2_shift;
		std::cout << "\tPLATFORM power limit #2: " 
			<< (((MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_enable_mask) >> MSR_PLATFORM_POWER_LIMIT_enable_shift) == 0 ? "disabled" : "enabled")
			<< " + clamping "
			<< (((MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_clamping_mask) >> MSR_PLATFORM_POWER_LIMIT_clamping_shift) == 0 ? "disabled" : "enabled")
			<< std::endl
			<< "\tPLATFORM power limit #2: " << (MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_power_mask) * unit["MSR_PLATFORM_POWER_LIMIT"] << " W\n"
			<< "\tPLATFORM power limit: " << (lock ? "locked" : "unlocked") 
			<< std::endl;
	}
*/
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
#elif defined HAVE_GEOPM_H
	MSR_RAPL_POWER_UNIT_bits = geopm_signal_to_field(pio.read_signal("MSR::RAPL_POWER_UNIT#", pio.signal_domain_type("MSR::RAPL_POWER_UNIT#"), 0));
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

#if defined HAVE_MSR_CORE_H || defined HAVE_X86_ADAPT_H || defined HAVE_GEOPM_H
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

void checkMsrWhitelist()
{
	std::map <uint64_t,uint64_t> currentWhitelist;
	currentWhitelist.insert(msr_whitelist.begin(), msr_whitelist.end());
	std::ifstream msrfile("/dev/cpu/msr_whitelist");
	if (!msrfile)
	{
		std::cerr << "LIBMSR ERROR: missing read access to /dev/cpu/msr_whitelist\n";
		exit(5);
	}

	//skip the first line with comment
	std::string comments;
	if(!std::getline (msrfile,comments))
	{
		std::cerr << "\nLIBMSR ERROR: Empty /dev/cpu/msr_whitelist - see msr-safe repository\n";
	}
	else
	{
		//read the list of registers
		std::map<uint64_t,uint64_t>::iterator it;
		uint64_t reg, mask;
		while (msrfile >> std::hex >> reg >> mask)
		{
			it = currentWhitelist.find(reg);
			if (it != currentWhitelist.end())
				currentWhitelist.erase(it);
		}
		msrfile.close();
		
		if (!currentWhitelist.empty())
		{
			std::cerr << "\n# MISSING REGISTERS IN /dev/cpu/msr_whitelist\n\t# MSR # Write Mask\n";
			for (std::pair<const uint64_t,uint64_t>& x: currentWhitelist)
				std::cerr << std::hex << "\t0x"<< x.first << "\t0x" << x.second << std::endl;
		}
	}
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
		if (access("/dev/cpu/0/msr_safe", W_OK) != 0 || access("/dev/cpu/0/msr_safe", R_OK) != 0)
			std::cerr << "LIBMSR ERROR: Missing read or write access to /dev/cpu/*/msr_safe\n";

	#else
	{
	#endif
		#ifdef HAVE_X86_ADAPT_H
		status = USING_ADAPT;
		if (x86_adapt_init())
		{
			std::cerr << "X86ADAPT ERROR: Unable to initialize\n";
		}
		else
		#endif
		{
			#ifdef HAVE_GEOPM_H
			status = USING_GEOPM;
			#elif defined HAVE_NUMA_H
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

	if (status == USING_MSR)
		checkMsrWhitelist();
	
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
