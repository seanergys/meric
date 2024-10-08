/**
 * Tool to provide information about the system parameters that can be tuned.
 **/
#include <math.h>
#include <map>
#include <vector>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <thread>
#include "../wrapper/environmentwrapper.h"
#include "../wrapper/raplwrapper.h"

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

#ifdef HAVE_NVML_H
	#include <nvml.h>
#endif

#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	#include <rocm_smi/rocm_smi.h>
#endif

////////////////////////////////////////////////////////////////////////////////
std::map<std::string, double> unit;
std::map <uint64_t,uint64_t> currentWhitelist;
std::string CPUname = "";
int CORES_PER_SOCKET = 0;

enum MODE	//this application keeps this order
{
	USING_MSR,
	USING_ADAPT,
	USING_GEOPM,
	USING_NONE
};
////////////////////////////////////////////////////////////////////////////////

void sysInfoSize(bool detailed = false)
{
	int SOCKETS_PER_NODE, THREADS_PER_CORE;
	std::vector <int> onlineThreads;
	meric::Environment::initHWSizes(SOCKETS_PER_NODE, CORES_PER_SOCKET, THREADS_PER_CORE, onlineThreads);
	meric::Environment::getCPUbrandString(CPUname);
	std::cout << "# SYSTEM INFORMATION\n"
	          << "\tCPU name:         " << CPUname << '\n'
	          << "\tSockets per node: " << SOCKETS_PER_NODE << '\n'
	          << "\tCores per socket: " << CORES_PER_SOCKET << '\n'
	          << "\tThreads per core: " << THREADS_PER_CORE;
	          
	if (onlineThreads.size() != SOCKETS_PER_NODE * CORES_PER_SOCKET * THREADS_PER_CORE)
	{
		std::sort (onlineThreads.begin(), onlineThreads.end());
		std::cout << "\n\tActive threads: ";
		for (int i=onlineThreads.size()-1; i>=0; i--)
			std::cout << onlineThreads[i] << " ";
	}
	std::cout << std::endl;

	if (detailed)
	{
		std::cout << std::endl;
		for (int socket = 0; socket < SOCKETS_PER_NODE; socket++)
		{
			std::cout << "\tSOCKET "<< socket <<":\n";
			for (const auto &core : meric::Environment::mapCoresToSocket[socket])
			{
				std::cout << "\t\tCore " << core << ":";
				for (const auto &thread : meric::Environment::mapThreadsToCore[core])
					std::cout << " " << thread;
				std::cout << std::endl;
			}
		}
	}
	
	unsigned int GPUdeviceCount = 0;
#ifdef HAVE_NVML_H
	nvmlReturn_t ret = nvmlDeviceGetCount(&GPUdeviceCount);
	if (GPUdeviceCount)
	{
		nvmlDevice_t deviceHandle;
		if (nvmlDeviceGetHandleByIndex(0, &deviceHandle) == NVML_SUCCESS)
		{
			// The name is an alphanumeric string that denotes a particular product, e.g. Tesla C2070. It will not exceed 50 characters in length (including the null terminator).
			const int GPU_NAME_LENGTH = 50;
			char GPUname [GPU_NAME_LENGTH];
			ret = nvmlDeviceGetName (deviceHandle, GPUname, GPU_NAME_LENGTH);
			std::cout << "\tGPU name:         " << GPUname << '\n';
		}
		std::cout << "\tGPUs per node:    " << GPUdeviceCount << '\n';
	}
#else
	DIR *dir;
	if ((dir = opendir ("/proc/driver/nvidia/gpus/")) != NULL)
	{
		struct dirent *ent;
		/* print all the files and directories within directory */
		std::string GPUname = "";
		while ((ent = readdir (dir)) != NULL)
		{
			if (ent->d_name[0] != '.')
			{
				GPUdeviceCount++;
				if (GPUname == "")
					GPUname = std::string(ent->d_name);
			}
		}
		closedir (dir);
		
		std::ifstream GPUinfoFile("/proc/driver/nvidia/gpus/"+GPUname+"/information");
		if (GPUinfoFile.is_open())
		{
			std::getline(GPUinfoFile, GPUname);
			GPUinfoFile.close();
			
			GPUname = GPUname.substr(GPUname.find(':'));
			std::size_t pos = GPUname.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
			if (pos != std::string::npos)
				GPUname = GPUname.substr(pos);
		}
		if (GPUname != "")
			std::cout << "\tGPU name:         " << GPUname << '\n';
		if (GPUdeviceCount)
			std::cout << "\tGPUs per node:    " << GPUdeviceCount << '\n';
		
		std::cout << "\n\tCompile MERIC with NVML to controll and monitor the GPUs\n";
	}
#endif

#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	rsmi_status_t status;
	rsmi_num_monitor_devices(&GPUdeviceCount);
	if (GPUdeviceCount > 0) {
		const int GPU_NAME_LENGTH = 50;
        char GPUname[50];

        status = rsmi_dev_name_get(0, GPUname, GPU_NAME_LENGTH);
        std::cout << "\tGPU name:         " << GPUname << '\n';

        std::cout << "\tGPUs per node:    " << GPUdeviceCount << '\n';
    }
#else

	//MISSING IMPLEMENTATION - obtain AMD GPU info without the compilation using rocm_smi64 if possible

#endif
}

bool sysInfoFrequencies(meric::systemLimits& limits, bool detailed = false)
{
	bool DVFS = false;
	bool ok = true;
// CORE FREQUENCY
	std::cout << "\n# CPU FREQUENCIES\n";
	std::ifstream frequencyBounderyFile;
	std::string scalingDriver = meric::Environment::getScalingDriver();
	if (scalingDriver != "")
	{
		std::cout << "\tCurrent scaling driver:      " << scalingDriver << std::endl;
	}

	frequencyBounderyFile.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
	if (frequencyBounderyFile.is_open())
	{
		std::string scalingGovernor = "";
		frequencyBounderyFile >> scalingGovernor;
		frequencyBounderyFile.close();
		std::cout << "\tCurrent scaling governor:    " << scalingGovernor << std::endl;
	}

	frequencyBounderyFile.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors");
	if (frequencyBounderyFile.is_open())
	{
		std::string availScalingGovernor = "";
		getline(frequencyBounderyFile, availScalingGovernor);
		frequencyBounderyFile.close();
		std::cout << "\tAvailable governors:         " << availScalingGovernor << std::endl;
	}

	bool HWP = false;
#ifdef _INTEL_
	std::string status;
	HWP = meric::Environment::isHWPactive(status);
	std::cout << "\tHardware controlled P-State: " << status << std::endl;
#endif
	
	if (!meric::Environment::isDVFSpossible())
	{
		std::cout << "\n\tCPU core frequency scaling (DVFS) NOT available\n";
		if (HWP)
		{
			std::cout << "\t\t grant write access to:\n"
					      << "\t\t - /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq\n"
					      << "\t\t - /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq\n";
		}
		else
		{
#if !defined(HAVE_MSR_SAFE) && !defined(HAVE_MSR_CORE_H) && !defined(HAVE_X86_ADAPT_H) && !defined(HAVE_CPUFREQ_H) && defined(__x86_64__)
			std::cout << "\t\t install one the following libraries:\n"
			          << "\t\t - msr_safe+libmsr / msr_safe / x86_adapt / cpufreq\n";
#endif
			if (scalingDriver == "intel_pstate")
			{
				std::cout << "\t\t grant write access to:\n"
					      << "\t\t - /sys/devices/system/cpu/intel_pstate/no_turbo\n"
					      << "\t\t - /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq\n"
					      << "\t\t or switch scaling driver to one that supports userspace scaling governor and\n";
			}
			struct stat info;
			if(stat("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", &info) == 0)
			{	
				std::cout << "\t\t grant write access to:\n"
					      << "\t\t - /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor\n";			          
				if (stat("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", &info) == 0)
				{
					std::cout << "\t\t - /sys/devices/system/cpu/cpu*/cpufreq/scaling_setspeed\n";
				}
			}
		}
		std::cout << std::endl;
	}
	
	long freq = -1;
	//files have the CPU core frequency in KHz
	frequencyBounderyFile.open("/sys/devices/system/cpu/cpufreq/boost");
	if (frequencyBounderyFile.is_open())
	{
		std::string turboInfo = "";
		frequencyBounderyFile >> turboInfo;
		frequencyBounderyFile.close();
		freq = std::stoi(turboInfo);
	}
	else if (scalingDriver == "intel_pstate")
	{
		frequencyBounderyFile.open("/sys/devices/system/cpu/intel_pstate/no_turbo");
		if (frequencyBounderyFile.is_open())
		{
			std::string turboInfo = "";
			frequencyBounderyFile >> turboInfo;
			frequencyBounderyFile.close();
			freq = std::stoi(turboInfo) == 0 ? 1 : 0;
		}
	}
#if defined _AMD_ && defined MSR_HWCR
	else if (!currentWhitelist.count(MSR_HWCR))
	{
		uint64_t MSR_HWCR_bits = 0x0;
		meric::read_msr(0, MSR_HWCR, &MSR_HWCR_bits);
		freq = (MSR_HWCR_bits & MSR_HWCR_boost_disable_mask) >> MSR_HWCR_boost_disable_shift ? 0 : 1;
	}
#endif
	
	if (freq == 0)
	{
		std::cout << "\tTurbo CPU core frequencies:  disabled\n";
	}
	else if (freq == 1)
	{
		std::cout << "\tTurbo CPU core frequencies:  ";
#if ( defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H ) && defined MSR_TURBO_RATIO_LIMIT && defined MSR_TURBO_RATIO_LIMIT_CORES
		uint64_t MSR_TURBO_RATIO_LIMIT_bits = 0x0;
		if (!currentWhitelist.count(MSR_TURBO_RATIO_LIMIT))
			meric::read_msr(0, MSR_TURBO_RATIO_LIMIT, &MSR_TURBO_RATIO_LIMIT_bits);
		uint64_t MSR_TURBO_RATIO_LIMIT_CORES_bits = 0x0;
		if (!currentWhitelist.count(MSR_TURBO_RATIO_LIMIT_CORES))
			meric::read_msr(0, MSR_TURBO_RATIO_LIMIT_CORES, &MSR_TURBO_RATIO_LIMIT_CORES_bits);
		
		std::vector <int> turboRatio;
		turboRatio.reserve(16);
		std::vector <int> turboCoresRange;
		turboCoresRange.reserve(16);
		std::vector<int>::iterator it = turboRatio.begin();

		for (int i=0; i<64/MSR_TURBO_RATIO_LIMIT_shift; i++)
		{
			turboRatio.push_back(MSR_TURBO_RATIO_LIMIT_CORES_bits & MSR_TURBO_RATIO_LIMIT_mask);
			MSR_TURBO_RATIO_LIMIT_CORES_bits = MSR_TURBO_RATIO_LIMIT_CORES_bits >> MSR_TURBO_RATIO_LIMIT_shift;
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

		it = turboRatio.begin();
		for (int i=0; i<64/MSR_TURBO_RATIO_LIMIT_shift; i++)
		{
			turboRatio.insert(it++, MSR_TURBO_RATIO_LIMIT_bits & MSR_TURBO_RATIO_LIMIT_mask);
			MSR_TURBO_RATIO_LIMIT_bits = MSR_TURBO_RATIO_LIMIT_bits >> MSR_TURBO_RATIO_LIMIT_shift;
		}
		uint64_t cores = 0;
		//int sn, cs, tc = 0;
		//std::vector<int> &online;
		//initHWSizes(sn, cs, tc, online); cores = cs; std::cout << "CORE"
		//core_config(&cores, NULL, NULL, NULL);
		cores = CORES_PER_SOCKET;
		int i = 0;
		while (i < turboRatio.size() && turboRatio[i] != 0 && turboCoresRange[i] <= cores)
		{
			std::cout << turboRatio[i]*100000 << "(" << turboCoresRange[i]<<") ";
			i++;
		}
		if (i > 0)
		{
			std::cout << "kHz(#cores)\n";
		}
		else
		{
			std::cout << "enabled\n";
		}
#elif defined HAVE_E_SMI_E_SMI_H
		freq = meric::Environment::getMaximumTurboFrequency();
		if (freq > 0)
		{
			std::cout << freq/1000 << " kHz\n";
		}
		else
		{
			std::cout << "enabled\n";
		}
#else
		std::cout << "enabled\n";
#endif		
	}
	else
	{
		frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
		if (frequencyBounderyFile.is_open())
		{
			std::string freqBound = "";
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
			freq = std::stol(freqBound) * 1000;
			std::cout << "\tMax CPU core frequency:      " << freq/1000 << " kHz\n";
			if (limits.maxCoreFreq != 0 && freq != limits.maxCoreFreq)
			{
				ok = false;
			}
		}
	}

	std::vector<long> availCF;
	meric::Environment::getAvailableCF(availCF); //may identify nominal frequency
	unsigned long nominalFreq = meric::Environment::getNominalFrequency();
	if (nominalFreq > 0)
	{
		std::cout << "\tNominal CPU core frequency:  " << nominalFreq/1000 << " kHz\n";
	}

	frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
	if (frequencyBounderyFile.is_open())
	{
		std::string freqBound = "";
		frequencyBounderyFile >> freqBound;
		frequencyBounderyFile.close();
		freq = std::stol(freqBound) * 1000;
		std::cout << "\tMin CPU core frequency:      " << freq/1000 << " kHz\n";
		if (freq != limits.minCoreFreq)
		{
			ok = false;
		}
	}

	if (availCF.size() > 0)
	{
		std::cout << "\tAvailable CPU core frequencies:  ";
		for(long CF : availCF)
			std::cout << CF/1000 << " " ;
		std::cout << "kHz\n";
	}
	
// IA32_ENERGY_PERF_BIAS
#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined IA32_ENERGY_PERF_BIAS
	if (ok)
	{
		uint64_t IA32_ENERGY_PERF_BIAS_bits = 0x0;
		if (!currentWhitelist.count(IA32_ENERGY_PERF_BIAS))
			meric::read_msr(0, IA32_ENERGY_PERF_BIAS, &IA32_ENERGY_PERF_BIAS_bits);
		if (IA32_ENERGY_PERF_BIAS_bits != limits.IA32_ENERGY_PERF_BIAS_default)
		{
			ok = false;
		}
	}
#endif

// UNCORE FREQUENCY
#ifdef _INTEL_
	#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_UNCORE_RATIO_LIMIT
		uint64_t MSR_UNCORE_RATIO_LIMIT_bits = 0x0;
		if (!currentWhitelist.count(MSR_UNCORE_RATIO_LIMIT))
		{
			meric::read_msr(0, MSR_UNCORE_RATIO_LIMIT, &MSR_UNCORE_RATIO_LIMIT_bits);
			freq = (MSR_UNCORE_RATIO_LIMIT_bits & MSR_UNCORE_RATIO_LIMIT_max_mask) * 100000000;
			std::cout << "\tMax CPU uncore frequency:    " << freq/1000 << " kHz\n";
			if (ok && freq != limits.maxUncoreFreq)
			{
				ok = false;
			}
			freq = ((MSR_UNCORE_RATIO_LIMIT_bits & MSR_UNCORE_RATIO_LIMIT_min_mask) >> MSR_UNCORE_RATIO_LIMIT_min_shift) * 100000000;
			std::cout << "\tMin CPU uncore frequency:    " << freq/1000 << " kHz\n";
			if (ok && freq != limits.minUncoreFreq)
			{
				ok = false;
			}
		}
		else
		{
			std::cout << "\tCPU uncore frequency limits: unknown\n";
		}
	#elif defined HAVE_X86_ADAPT_H
		int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
		int adaptHandlerUncore = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO");
		uint64_t setting = 0;
		x86_adapt_get_setting(fd, adaptHandlerUncore, &setting);
		freq = setting * 100000000;
		std::cout << "\tMax CPU uncore frequency:    " << freq/1000 << " kHz\n";
		if (ok && freq != limits.maxUncoreFreq)
		{
			ok = false;
		}
			
		adaptHandlerUncore = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MIN_RATIO");
		x86_adapt_get_setting(fd, adaptHandlerUncore, &setting);
		if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
			std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
		freq = setting * 100000000;
		std::cout << "\tMin CPU uncore frequency:    " << freq/1000 << " kHz\n";
		if (ok && freq != limits.minUncoreFreq)
		{
			ok = false;
		}
	#elif defined HAVE_GEOPM_H
		freq = pio.read_signal("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", pio.signal_domain_type("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO"), 0);
		std::cout << "\tMax CPU uncore frequency:    " << freq/1000 << " kHz\n";
		if (ok && freq != limits.maxUncoreFreq)
		{
			ok = false;
		}
		
		freq = pio.read_signal("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", pio.signal_domain_type("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO"), 0);
		std::cout << "\tMin CPU uncore frequency:    " << freq/1000 << " kHz\n";
		if (ok && freq != limits.minUncoreFreq)
		{
			ok = false;
		}
	#else
		std::cout << "\n# CPU uncore frequency NOT available - install libmsr or x86_adapt\n";
	#endif
#elif defined __arm__ //Jetson memory frequencies
	frequencyBounderyFile.open ("/sys/kernel/debug/clock/emc/max");
	if (frequencyBounderyFile.is_open())
	{
		std::string freqBound = "";
		frequencyBounderyFile >> freqBound;
		frequencyBounderyFile.close();
		freq = std::stol(freqBound);
		std::cout << "\tMax memory frequency:        " << freq << " Hz\n";
		if (freq != limits.maxUncoreFreq)
		{
			ok = false;
		}
	}

	frequencyBounderyFile.open ("/sys/kernel/debug/clock/emc/min");
	if (frequencyBounderyFile.is_open())
	{
		std::string freqBound = "";
		frequencyBounderyFile >> freqBound;
		frequencyBounderyFile.close();
		freq = std::stol(freqBound);
		std::cout << "\tMin memory frequency:        " << freq << " Hz\n";
		if (freq != limits.maxUncoreFreq)
		{
			ok = false;
		}
	}
	
	frequencyBounderyFile.open("/sys/kernel/debug/clock/emc/possible_rates");
	if (frequencyBounderyFile.is_open())
	{
		std::string freqBound = "";
		getline(frequencyBounderyFile, freqBound);
		frequencyBounderyFile.close();
		std::cout << "\tAvailable memory frequencies:    " << freqBound << " Hz\n";
	}
#endif


#ifdef HAVE_NVML_H
	nvmlDevice_t deviceHandle;
	if (nvmlDeviceGetHandleByIndex(0, &deviceHandle) == NVML_SUCCESS)
	{
		uint memFreqCount = 0;
		uint gpuFreqCount = 0;
		uint * memFreq = NULL;
		uint * gpuFreq = NULL;
		unsigned int freqSMmax = 0;

		if(nvmlDeviceGetSupportedMemoryClocks(deviceHandle, &memFreqCount, memFreq) == NVML_ERROR_INSUFFICIENT_SIZE)
		{
			std::cout << "\n# GPU FREQUENCIES\n";

			memFreq = (uint *)malloc(memFreqCount * sizeof(uint));
			if(nvmlDeviceGetSupportedMemoryClocks(deviceHandle, &memFreqCount, memFreq) == NVML_SUCCESS)
			{
				for(uint i = 0; i < memFreqCount; i++)
				{
					if(nvmlDeviceGetSupportedGraphicsClocks(deviceHandle, memFreq[i], &gpuFreqCount, gpuFreq) == NVML_ERROR_INSUFFICIENT_SIZE)
					{
						gpuFreq = (uint *)malloc(gpuFreqCount * sizeof(uint));
						if(nvmlDeviceGetSupportedGraphicsClocks(deviceHandle, memFreq[i], &gpuFreqCount, gpuFreq) == NVML_SUCCESS)
						{
							std::cout << "\tMemory: " << memFreq[i]*1000 << " kHz\tSM: " <<  gpuFreq[0]*1000 << " - " << gpuFreq[gpuFreqCount-1]*1000 << " kHz (" << gpuFreqCount<< " steps)\n";
							freqSMmax = gpuFreq[0];
							if (detailed)
							{
								std::cout << "\t\t";
								for (int f=0; f<gpuFreqCount; f++)
									std::cout << gpuFreq[f] << " ";
								std::cout << "MHz\n";
							}
						}
						free(gpuFreq);
						gpuFreqCount = 0;
					}
				}
			}
			free(memFreq);
		}
		
		unsigned int freqMem, freqSM, freq = 0;
		if(nvmlDeviceGetDefaultApplicationsClock(deviceHandle, NVML_CLOCK_MEM, &freqMem) == NVML_SUCCESS &&
		   nvmlDeviceGetDefaultApplicationsClock(deviceHandle, NVML_CLOCK_SM, &freqSM)   == NVML_SUCCESS)
		{
			std::cout << "\tDefault memory frequency: " << freqMem*1000 << " kHz\n"
			          << "\tNominal streaming multiprocessor frequency: " << freqSM*1000 << " kHz\n";
			if (nvmlDeviceGetApplicationsClock(deviceHandle, NVML_CLOCK_MEM, &freq) == NVML_SUCCESS && freq != freqMem &&
			    nvmlDeviceGetApplicationsClock(deviceHandle, NVML_CLOCK_SM, &freq)  == NVML_SUCCESS && freq != freqSM)
			{
				ok = false;
			}
		}
		
		if (nvmlDeviceGetClock(deviceHandle, NVML_CLOCK_MEM, NVML_CLOCK_ID_APP_CLOCK_TARGET, &freq) == NVML_SUCCESS && freq != freqMem)
		{
			std::cout << "\tCurrent memory frequency: " << freq*1000 << " kHz\n";
		}
		if (nvmlDeviceGetClock(deviceHandle, NVML_CLOCK_SM, NVML_CLOCK_ID_APP_CLOCK_TARGET, &freq) == NVML_SUCCESS && freq != freqSMmax)
		{
			std::cout << "\tStreaming multiprocessor frequency limit:   " << freq*1000 << " kHz\n";
		}
	}

	const unsigned int driverVersionLength = 256;
	char driverVersion [driverVersionLength];
	if (nvmlSystemGetDriverVersion (driverVersion, driverVersionLength) == NVML_SUCCESS && std::stod(driverVersion) < 465)
	{
		nvmlEnableState_t isRestricted;
		if (nvmlDeviceGetAPIRestriction(deviceHandle, NVML_RESTRICTED_API_SET_APPLICATION_CLOCKS, &isRestricted) == NVML_SUCCESS && isRestricted != NVML_FEATURE_DISABLED)
		{
			std::cout << "\n\tGPU frequencies tuning NOT allowed - execute `sudo nvidia-smi -acp UNRESTRICTED`\n";
		}
	}
#endif


#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	rsmi_frequencies_t sysFreqs;
	rsmi_frequencies_t socFreqs;
	rsmi_frequencies_t memFreqs;
	rsmi_status_t ret_sys, ret_soc, ret_mem;
	
	ret_sys = rsmi_dev_gpu_clk_freq_get(0, RSMI_CLK_TYPE_SYS, &sysFreqs);
	ret_soc = rsmi_dev_gpu_clk_freq_get(0, RSMI_CLK_TYPE_SOC, &socFreqs);
	ret_mem = rsmi_dev_gpu_clk_freq_get(0, RSMI_CLK_TYPE_MEM, &memFreqs);
	

	std::cout << "\n# GPU FREQUENCIES\n";

 	if(ret_sys == RSMI_STATUS_SUCCESS){
		std::cout << "\tAvailable system frequencies:               " << sysFreqs.frequency[0] / 1000 << " - " <<   sysFreqs.frequency[sysFreqs.num_supported - 1] / 1000  << " kHz (" << sysFreqs.num_supported << " steps)\n";
		if(detailed || sysFreqs.num_supported <= 7){
			std::cout << "\t\t";		
			for(uint32_t i = 0; i < sysFreqs.num_supported; i++){
				std::cout << sysFreqs.frequency[i] / 1e6  << " ";
			}
			std::cout << "MHz\n";
		}
	}

	if(ret_soc == RSMI_STATUS_SUCCESS){
		if(socFreqs.num_supported > 1){
			std::cout << "\tAvailable SOC (System on Chip) frequencies: " << socFreqs.frequency[0] / 1000 << " - " <<   socFreqs.frequency[socFreqs.num_supported - 1] / 1000  << " kHz (" << socFreqs.num_supported << " steps)\n";
			if(detailed || socFreqs.num_supported <= 7){
				std::cout << "\t\t";
				for(uint32_t i = 0; i < socFreqs.num_supported; i++){
					std::cout << socFreqs.frequency[i] / 1e6  << " ";
				}
				std::cout << "MHz\n";
			}
		}else{
			std::cout << "\tSOC (System on Chip) frequency:             " << socFreqs.frequency[0] / 1000 << " kHz\n";
		}	
	}

	if(ret_mem == RSMI_STATUS_SUCCESS){
		if(memFreqs.num_supported > 1){
			std::cout << "\tAvailable memory frequencies:               " << memFreqs.frequency[0] / 1000 << " - " << memFreqs.frequency[memFreqs.num_supported - 1] / 1000  << " kHz (" << memFreqs.num_supported << " steps)\n";
			if(detailed || memFreqs.num_supported <= 7){
				std::cout << "\t\t";
				for(uint32_t i = 0; i < memFreqs.num_supported; i++){
					std::cout << memFreqs.frequency[i] / 1e6  << " ";
				}
				std::cout << "MHz\n";
			}
		}
		std::cout << "\tDefault memory frequency: " << memFreqs.frequency[memFreqs.num_supported - 1] / 1000  << " kHz \n";
		std::cout << "\tCurrent memory frequency: " << memFreqs.frequency[memFreqs.current] / 1000  << " kHz \n";
	}

	if(ret_sys == RSMI_STATUS_SUCCESS)
		std::cout << "\tSystem frequency limit:   " << sysFreqs.frequency[sysFreqs.num_supported - 1] / 1000 << " kHz\n";	


#endif

	return ok;
}

bool sysInfoPowercap(meric::systemLimits& limits)
{
	bool ok = true;
#ifdef _INTEL_	//currently only Intel power capping supported
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
		std::cout << "\n# CPU RAPL power capping NOT available\n";
	}
	else
	{
// power capping limits ////////////////////////////////////////////////////////
		std::cout << "\n# CPU POWER LIMITS\n";	
	#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_PKG_POWER_INFO && defined MSR_DRAM_POWER_INFO
	// PKG
		uint64_t MSR_PKG_DRAM_POWER_INFO_bits = 0x0;
		if (!currentWhitelist.count(MSR_PKG_POWER_INFO))
			meric::read_msr(0, MSR_PKG_POWER_INFO, &MSR_PKG_DRAM_POWER_INFO_bits);

		std::cout 
			<< "\tRAPL time window unit: " << meric::RAPL::getUnit(RAPL_POWER_LIMIT_TIMEWINDOW)*1000000 << " us\n"
			<< "\tPKG max power limit:   " 
			<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_power_mask) >> MSR_PKG_DRAM_POWER_INFO_max_power_shift) * meric::RAPL::getUnit(RAPL_PCKG_POWER_LIMIT) << " W\n"
			
			<< "\tPKG min power limit:   " 
			<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_min_power_mask) >> MSR_PKG_DRAM_POWER_INFO_min_power_shift) * meric::RAPL::getUnit(RAPL_PCKG_POWER_LIMIT) << " W\n"

	//		<< "\tPKG max time window:   " 
	//		<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_time_mask) >> MSR_PKG_DRAM_POWER_INFO_max_time_shift) * meric::RAPL::getUnit(RAPL_POWER_LIMIT_TIMEWINDOW) << " s\n"
			;

	// DRAM
		if (!currentWhitelist.count(MSR_DRAM_POWER_INFO))
			meric::read_msr(0, MSR_DRAM_POWER_INFO, &MSR_PKG_DRAM_POWER_INFO_bits);
		std::cout 
			<< "\tDRAM max power limit:  "
			<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_power_mask) >> MSR_PKG_DRAM_POWER_INFO_max_power_shift) * meric::RAPL::getUnit(RAPL_RAM_POWER_LIMIT) << " W\n"

			<< "\tDRAM min power limit:  " 
			<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_min_power_mask) >> MSR_PKG_DRAM_POWER_INFO_min_power_shift) * meric::RAPL::getUnit(RAPL_RAM_POWER_LIMIT) << " W\n"

	//		<< "\tDRAM max time window:  " 
	//		<< ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_time_mask) >> MSR_PKG_DRAM_POWER_INFO_max_time_shift) * meric::RAPL::getUnit(RAPL_POWER_LIMIT_TIMEWINDOW) << " s\n"
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

	#if defined __x86_64 && !defined HAVE_MSR_SAFE && !defined HAVE_MSR_CORE_H && ! defined HAVE_X86_ADAPT_H //currently supported only intel
		std::cout << "\n\tchanging RAPL power capping limits NOT available - install msr_safe+libmsr or msr_safe or x86_adapt\n";
	#endif

// default power capping ///////////////////////////////////////////////////////
		std::cout << "\n# DEFAULT CPU POWER LIMITS\n";
		uint64_t X, Y, Z, F;

	#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_X86_ADAPT_H || defined HAVE_GEOPM_H
		// PKG
			uint64_t MSR_PKG_POWER_LIMIT_bits = 0x0;
		#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_PKG_POWER_LIMIT
			if (!currentWhitelist.count(MSR_PKG_POWER_LIMIT))
			{
				meric::read_msr(0, MSR_PKG_POWER_LIMIT, &MSR_PKG_POWER_LIMIT_bits);
			}
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
				if ((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_lock_mask) != 0)
				{
					std::cout << "\n\tchanging RAPL PKG power capping limits NOT available - requires system (RAPL LOCK bit) reset\n\n";
				}
				std::cout << "\tPKG power limit #1: " 
					<< (((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_enable_mask) >> MSR_PKG_POWER_LIMIT_enable_shift) == 0 ? "disabled" : "enabled")
					<< " + clamping "
					<< (((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_clamping_mask) >> MSR_PKG_POWER_LIMIT_clamping_shift ) == 0 ? "disabled" : "enabled")
					<< std::endl
					<< "\tPKG power limit #1: " << (MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_mask) * meric::RAPL::getUnit(RAPL_PCKG_POWER_LIMIT) << " W\n";

				Y = ((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_time_y_mask) >> MSR_PKG_POWER_LIMIT_time_y_shift);
				Z = ((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_time_z_mask) >> MSR_PKG_POWER_LIMIT_time_z_shift);
				std::cout << "\tPKG time window #1: "
						  << pow(2,Y) * (1.0 + Z/4.0)*meric::RAPL::getUnit(RAPL_POWER_LIMIT_TIMEWINDOW)
						  << " s\n";

		#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_PKG_POWER_LIMIT
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
					<< "\tPKG power limit #2: " << (MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_mask) * meric::RAPL::getUnit(RAPL_PCKG_POWER_LIMIT) << " W\n";

				Y = ((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_time_y_mask) >> MSR_PKG_POWER_LIMIT_time_y_shift);
				Z = ((MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_time_z_mask) >> MSR_PKG_POWER_LIMIT_time_z_shift);
				std::cout << "\tPKG time window #2: "
						  << pow(2,Y) * (1.0 + Z/4.0) *meric::RAPL::getUnit(RAPL_POWER_LIMIT_TIMEWINDOW)
						  << " s\n";
			}

		// DRAM
			uint64_t MSR_DRAM_POWER_LIMIT_bits = 0x0;
		#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_DRAM_POWER_LIMIT
			if (!currentWhitelist.count(MSR_DRAM_POWER_LIMIT))
				meric::read_msr(0, MSR_DRAM_POWER_LIMIT, &MSR_DRAM_POWER_LIMIT_bits);
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
					<< "\tDRAM power limit:   " << (MSR_DRAM_POWER_LIMIT_bits & MSR_DRAM_POWER_LIMIT_mask) * meric::RAPL::getUnit(RAPL_RAM_POWER_LIMIT) << " W\n";

				F = 1.0 + ((MSR_DRAM_POWER_LIMIT_bits & MSR_DRAM_POWER_LIMIT_time_f_mask) >> MSR_DRAM_POWER_LIMIT_time_f_shift)/10.0;
				Y = ((MSR_DRAM_POWER_LIMIT_bits & MSR_DRAM_POWER_LIMIT_time_y_mask) >> MSR_DRAM_POWER_LIMIT_time_y_shift);
				std::cout << "\tDRAM time window:   "
						  << pow(2,Y) * F * meric::RAPL::getUnit(RAPL_POWER_LIMIT_TIMEWINDOW)
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
		uint64_t MSR_PLATFORM_ENERGY_COUNTER_bits = 0x0;
	#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_PLATFORM_ENERGY_COUNTER
		if (!currentWhitelist.count(MSR_PLATFORM_ENERGY_COUNTER))
		{
			meric::read_msr(0, MSR_PLATFORM_ENERGY_COUNTER, &MSR_PLATFORM_ENERGY_COUNTER_bits);
			if (!MSR_PLATFORM_ENERGY_COUNTER_bits)
			{
				usleep(2);
				meric::read_msr(0, MSR_PLATFORM_ENERGY_COUNTER, &MSR_PLATFORM_ENERGY_COUNTER_bits);
			}
		}
	#endif

		if (MSR_PLATFORM_ENERGY_COUNTER_bits)
		{
			uint64_t MSR_PLATFORM_POWER_LIMIT_bits = 0x0;
	#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_PLATFORM_POWER_LIMIT
			if (!currentWhitelist.count(MSR_PLATFORM_POWER_LIMIT))
				meric::read_msr(0, MSR_PLATFORM_POWER_LIMIT, &MSR_PLATFORM_POWER_LIMIT_bits);
			if (MSR_PLATFORM_POWER_LIMIT_bits)
			{
				std::cout << "\tPLATFORM power limit #1: " 
					<< (((MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_enable_mask) >> MSR_PLATFORM_POWER_LIMIT_enable_shift) == 0 ? "disabled" : "enabled")
					<< " + clamping "
					<< (((MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_clamping_mask) >> MSR_PLATFORM_POWER_LIMIT_clamping_shift ) == 0 ? "disabled" : "enabled")
					<< std::endl
					<< "\tPLATFORM power limit #1: " << (MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_power_mask) * meric::RAPL::getUnit(RAPL_PLATFORM_POWER_LIMIT) << " W\n";

				Y = ((MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_time_y_mask) >> MSR_PLATFORM_POWER_LIMIT_time_y_shift);
				X = ((MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_time_x_mask) >> MSR_PLATFORM_POWER_LIMIT_time_x_shift);
				std::cout << "\tPLATFORM time window: "
						  << pow(2,Y) * (1.0 + X/4.0) * meric::RAPL::getUnit(RAPL_POWER_LIMIT_TIMEWINDOW)
						  << " s\n";
				const bool lock = ((MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_lock_mask) >> MSR_PLATFORM_POWER_LIMIT_lock_shift);

				MSR_PLATFORM_POWER_LIMIT_bits = MSR_PLATFORM_POWER_LIMIT_bits >> MSR_PLATFORM_POWER_LIMIT_power2_shift;
				std::cout << "\tPLATFORM power limit #2: " 
					<< (((MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_enable_mask) >> MSR_PLATFORM_POWER_LIMIT_enable_shift) == 0 ? "disabled" : "enabled")
					<< " + clamping "
					<< (((MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_clamping_mask) >> MSR_PLATFORM_POWER_LIMIT_clamping_shift) == 0 ? "disabled" : "enabled")
					<< std::endl
					<< "\tPLATFORM power limit #2: " << (MSR_PLATFORM_POWER_LIMIT_bits & MSR_PLATFORM_POWER_LIMIT_power_mask) * meric::RAPL::getUnit(RAPL_PLATFORM_POWER_LIMIT) << " W\n"
					<< "\tPLATFORM power limit: " << (lock ? "locked" : "unlocked") 
					<< std::endl;
			}
	#endif
		}
	}
#endif // _INTEL_

#ifdef HAVE_E_SMI_E_SMI_H //_AMD_
	std::cout << "\n# CPU POWER LIMITS\n";
	uint32_t CPUPowerLimit;
	if (!esmi_socket_power_cap_max_get(0, &CPUPowerLimit))
	{
		std::cout << "\tCPU max power limit:     " << CPUPowerLimit/1000 << " W\n";
	}

	if (!esmi_socket_power_cap_get(0, &CPUPowerLimit))
	{
		std::cout << "\tCPU power limit:         " << CPUPowerLimit/1000 << " W\n";
	}
#endif

#ifdef HAVE_OCC
	
	std::ifstream pwrCapFile;
	std::string pwrCapInfo = "";

	pwrCapFile.open("/sys/firmware/opal/powercap/system-powercap/powercap-max");
	if (pwrCapFile.is_open())
	{
		pwrCapFile >> pwrCapInfo;
		pwrCapFile.close();

		std::cout << "\n# NODE POWER LIMITS\n"
		          << "\tNode max power limit:     " << pwrCapInfo << " W\n";

		pwrCapFile.open("/sys/firmware/opal/powercap/system-powercap/powercap-min");
		if (pwrCapFile.is_open())
		{
			pwrCapFile >> pwrCapInfo;
			pwrCapFile.close();
		}
		std::cout << "\tNode min power limit:     " << pwrCapInfo << " W\n";
		
		pwrCapFile.open("/sys/firmware/opal/powercap/system-powercap/powercap-current");
		if (pwrCapFile.is_open())
		{
			pwrCapFile >> pwrCapInfo;
			pwrCapFile.close();
		}
		std::cout << "\tNode power limit:         " << pwrCapInfo << " W\n";
	}
#endif

// GPU
#ifdef HAVE_NVML_H
	nvmlDevice_t deviceHandle;
	if (nvmlDeviceGetHandleByIndex(0, &deviceHandle) == NVML_SUCCESS)
	{
		std::cout << "\n# GPU POWER LIMITS\n";
		uint minPowerLimit, maxPowerLimit;
		if (nvmlDeviceGetPowerManagementLimitConstraints(deviceHandle, &minPowerLimit, &maxPowerLimit) == NVML_SUCCESS)
		{
			std::cout << "\tGPU max power limit:     " << maxPowerLimit/1000 << " W\n"
			          << "\tGPU min power limit:     " << minPowerLimit/1000 << " W\n";
		}
		
		if (nvmlDeviceGetPowerManagementDefaultLimit(deviceHandle, &maxPowerLimit) == NVML_SUCCESS)
		{
			std::cout << "\tGPU default power limit: " << maxPowerLimit/1000 << " W\n";
		}
		
		if (nvmlDeviceGetPowerManagementLimit(deviceHandle, &minPowerLimit) == NVML_SUCCESS && minPowerLimit != maxPowerLimit)
		{
			std::cout << "\tGPU power limit:         " << minPowerLimit/1000 << " W\n";
			ok = false;
		}
	}
#endif

#ifdef HAVE_ROCM_SMI_ROCM_SMI_H

	uint64_t min_power_limit, max_power_limit, current_power_limit, default_power_limit;
	rsmi_status_t ret;

	ret = rsmi_dev_power_cap_range_get(0, 0, &max_power_limit, &min_power_limit);
	if (ret == RSMI_STATUS_SUCCESS) {
		std::cout << "\n# GPU POWER LIMITS\n";
		std::cout << "\tGPU max power limit:     " <<  max_power_limit / 1000000 << " W\n";
		std::cout << "\tGPU min power limit:     " << min_power_limit / 1000000 << " W\n" ;
	}

	ret = rsmi_dev_power_cap_default_get(0, &default_power_limit);
	if (ret == RSMI_STATUS_SUCCESS) {
		std::cout << "\tGPU default power limit: " <<  default_power_limit / 1000000 << " W\n";
	}

	ret = rsmi_dev_power_cap_get(0, 0, &current_power_limit);
	if (ret == RSMI_STATUS_SUCCESS) {
		std::cout << "\tGPU power limit:         " << current_power_limit / 1000000 << " W\n";
	}

#endif




	return ok;
}

void sysEnergyMeasurement()
{
	int systems = 0;
	std::cout << "\n# AVAILABLE POWER MONITORING SYSTEMS\n";

#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_X86_ADAPT_H || defined HAVE_GEOPM_H
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

#ifdef HAVE_NVML_H
	unsigned long long energy;
	nvmlDevice_t deviceHandle;
	if (nvmlDeviceGetHandleByIndex(0, &deviceHandle) == NVML_SUCCESS && nvmlDeviceGetTotalEnergyConsumption(deviceHandle, &energy) == NVML_SUCCESS)
	{
		std::cout << "\tNVML\n";
		systems++;
	}
#endif

#ifdef HAVE_OCC
	std::cout << "\tOCC\n";
	systems++;
#endif

#if defined __aarch64__ && defined HAVE_LINUX_PERF_EVENT_H
	if (CPUname == "Fujitsu A64FX")
	{
		std::cout << "\tA64FX\n";
		systems++;
	}
#endif

	if (systems == 0)
		std::cout << "\tNONE\n";
}

void checkMsrWhitelist()
{
	std::string whitelistName;
	if (access("/dev/cpu/msr_whitelist", F_OK ) != -1 )
		whitelistName = "/dev/cpu/msr_whitelist";
	else if (access("/dev/cpu/msr_allowlist", F_OK ) != -1 ) //from v1.5.0
		whitelistName = "/dev/cpu/msr_allowlist";
	else
	{
		std::cerr << "\nMSR_SAFE ERROR: No allowlist file in /dev/cpu/\n";
		return;
	}
	currentWhitelist.insert(msr_whitelist.begin(), msr_whitelist.end());
	std::ifstream msrfile(whitelistName);
	if (!msrfile)
	{
		std::cerr << "MSR_SAFE ERROR: missing read access to "<< whitelistName <<"\n";
		exit(5);
	}

	//skip the first line with comment
	std::string comments;
	if(!std::getline (msrfile,comments))
	{
		std::cerr << "\nMSR_SAFE ERROR: Empty "<< whitelistName <<" - see msr-safe repository\n";
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
			std::cerr << "\n# MISSING REGISTERS IN "<< whitelistName <<"\n\t# MSR # Write Mask\n";
			for (std::pair<const uint64_t,uint64_t>& x: currentWhitelist)
				std::cerr << std::hex << "\t0x"<< x.first << "\t0x" << x.second << std::endl;
		}
	}
	std::cerr << std::endl;
}

//****************************************************************************//
int main(int argc, char **argv)
{
	bool restore = false;
	bool detailedInfo = false;
	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg == "-r")
			restore = true;
		else if (arg == "-t")
			detailedInfo = true;
		else
		{
			std::cerr << "Utility prints information about the system topology and HW knobs ranges\n"
			          << "Parameters:\n"
			          << "\t-t = prints detailed topology of the node\n"
			          << "\t-r = if the system is not in the default setting, the tool restore it\n\n";
			return 1;
		}	
	}

	MODE status = USING_NONE;
	#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H
	status = USING_MSR;
		#ifdef HAVE_MSR_CORE_H
		if (init_msr())
			std::cerr << "LIBMSR ERROR: Unable to initialize\n";
		#endif
	#elif defined HAVE_X86_ADAPT_H
	status = USING_ADAPT;
	if (x86_adapt_init())
		std::cerr << "X86ADAPT ERROR: Unable to initialize\n";
	#elif defined HAVE_GEOPM_H
	status = USING_GEOPM;
	#endif
	
	#ifdef HAVE_E_SMI_E_SMI_H
	if (esmi_init() != ESMI_SUCCESS)
		std::cerr << "ESMI ERROR: Unable to initialize\n";
	#endif
	
	#ifdef HAVE_NVML_H
	nvmlInit();
	#endif
	
	#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	rsmi_init(0);
	#endif


	if (status == USING_MSR)
		checkMsrWhitelist();

	bool systemInTheDefaultSettings = true;
	timespec modifTime;
	std::string restoreJson;
	bool restoreFile = meric::Environment::readRestoreFile(restoreJson, modifTime);
	meric::systemLimits limits = meric::Environment::getSystemSettings();
	meric::RAPL::initCountersUnits(); //not necessary if meric::Environment::init() would have been called before
	sysInfoSize(detailedInfo);
	systemInTheDefaultSettings = sysInfoFrequencies(limits, detailedInfo);

	systemInTheDefaultSettings = sysInfoPowercap(limits) && systemInTheDefaultSettings;
	sysEnergyMeasurement();
	
	if (restoreFile)
	{
		if (restore)
		{
			meric::RegionParameters defaultParameter;
			meric::Environment::init(meric::SYS_OTHER, defaultParameter, 0);
			meric::Environment::restoreSystemSettings(true);
		}
		else if (!systemInTheDefaultSettings)
		{
			std::cerr << "\n\tWARNING: system not in the default settings!"
				      << "\n\trun systemInfo with parameter '-r' to restore the settings\n";
		}
	}

	#ifdef HAVE_NVML_H
	nvmlShutdown();
	#endif

	#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	rsmi_shut_down();
	#endif	

	#ifdef HAVE_E_SMI_E_SMI_H
	esmi_exit();
	#endif
	
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
