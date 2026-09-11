#include "environmentwrapper.h"
#include "raplwrapper.h"
#include "nvmlwrapper.h"
#include "rocmwrapper.h"
#include "../meric/meric.h"

using namespace meric;

systemLimits Environment::defaultSettings;

int Environment::SOCKETS_PER_NODE = 1;
int Environment::CORES_PER_SOCKET = 1;
int Environment::THREADS_PER_CORE = 1;
std::map<int, std::vector<int>> Environment::mapThreadsToCore;
std::map<int, std::vector<int>> Environment::mapCoresToSocket;
std::map<int, std::vector<uint64_t> > Environment::mapCoresToTIDs;
SYSTEM_TYPE Environment::systemType = SYS_OTHER;

std::string Environment::scalingGovernor = "";
std::string Environment::scalingDriver = "";

bool Environment::usedDVFS = false;
bool Environment::usedUFS = false;
bool Environment::usedPWRCAP = false;
bool Environment::usedGPUDVFS = false;
bool Environment::usedGPUPWRCAP = false;
DVFS_WAY Environment::activatedDVFS = NO_DVFS;
UFS_WAY  Environment::activatedUFS  = UFS_WAY_NONE;
bool Environment::activatedPWRCAP = false;
GPU_TUNING_WAY Environment::activatedGPUtuning = NO_TUNING;
#if defined HAVE_NVML_H || defined HAVE_ROCM_SMI_ROCM_SMI_H
uint Environment::currentGPUMemFreq = 0;
uint Environment::currentGPUSMFreq = 0;
#endif

#ifdef __x86_64
/**
 * Provides the processor name, reads it from register
 **/
void Environment::getCPUbrandString(std::string & CPUname)
{
	const int STRING_SIZE=256;
	char buff[STRING_SIZE];

	unsigned int regs[4];
	unsigned int size_reg = sizeof(unsigned int);
	memset(buff, 0x0, sizeof(char) * STRING_SIZE);

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
	CPUname = std::string(buff);
}


void Environment::getCPUFamilyModelString(std::string &CPUFamilyModelOut)
{
	unsigned int regs[4];
	regs[eax] = 0x1;
	regs[ebx] = 0x0;
	regs[ecx] = 0x0;
	regs[edx] = 0x0;
	CPUID(regs);

	uint32_t model = (regs[eax] >> 4) & 0xF;
	uint32_t family = (regs[eax] >> 8) & 0xF;
	uint32_t extended_model = (regs[eax] >> 16) & 0xF;
	uint32_t extended_family = (regs[eax] >> 20) & 0xFF;

    uint32_t model_synth = model;
    uint32_t family_synth = family;

	// Compute actual family and model
	if (family == 0xF) {
		family_synth += extended_family;
	}
	if (family == 0x6 || family == 0xF) {
		model_synth += (extended_model << 4);
	}
 
	std::ostringstream oss;
	oss << std::hex << std::uppercase << std::setfill('0');
	oss << std::setw(2) << family_synth << "_";
	oss << std::setw(2) << model_synth << "H";

	CPUFamilyModelOut = oss.str();

	#if defined(_INTEL_)
    	CPUFamilyModelOut = std::string("INTEL_") + CPUFamilyModelOut;
	#elif defined(_AMD_) || defined(__amd64__)
		CPUFamilyModelOut = std::string("AMD_") + CPUFamilyModelOut;
	#endif

}


#else
/**
 * Parse /proc/cpuinfo for the processor name, if not available, than creates internal name to recognise the hardware
 **/
void Environment::getCPUbrandString(std::string & CPUname)
{
	std::ifstream CPUinfoFile;
	CPUname = "";
	std::string line;

	CPUinfoFile.open ("/proc/cpuinfo");
	if (CPUinfoFile.is_open())
	{
		while (getline(CPUinfoFile,line))
		{
			std::string label = line.substr(0,10);
			std::for_each(label.begin(), label.end(), [](char & c){ c = ::tolower(c); });

			if (label == "model name")
			{
				CPUname = line.substr(line.find(':')+2);
				break;
			}
		}
		
		if (CPUname == "")
		{
			CPUinfoFile.clear();
			CPUinfoFile.seekg(0);
			
			while (getline(CPUinfoFile,line))
			{
				std::string label = line.substr(0,3);
				std::for_each(label.begin(), label.end(), [](char & c){ c = ::tolower(c); });
				if (label.substr(0,3) == "cpu")
				{
					CPUname = line.substr(line.find(':')+2);
					break;
				}
			}
		}
		
		//0x46 is identifier assigned to Fujitsu, which they use as CPU name
		//https://github.com/fujitsu/A64FX/blob/master/doc/A64FX_identification_method_v1_EN.pdf
		if (CPUname == "0x46")
		{
			CPUname = "Fujitsu";

			CPUinfoFile.clear();
			CPUinfoFile.seekg(0);
			
			while (getline(CPUinfoFile,line))
			{
				std::string label = line.substr(0,8);
				std::for_each(label.begin(), label.end(), [](char & c){ c = ::tolower(c); });
				if (label == "cpu part")
				{
					std::string model = line.substr(line.find(':')+2);
					char* test;
					long model_id = strtol(model.c_str(), &test, 16);
					if (*test == 0 && model_id == 0x1)
					{
						CPUname += " A64FX";
					}
					else
					{
						CPUname += " " + model;
					}
					break;
				}
			}
		}
		// AArch64 MIDR_EL1, Main ID Register
		// https://developer.arm.com/documentation/107734/0002/AArch64-registers/AArch64-Identification-registers-summary/MIDR-EL1--Main-ID-Register?lang=en
		else if (CPUname == "0x41")
		{
			CPUname = "ARM";
			CPUinfoFile.clear();
			CPUinfoFile.seekg(0);
			std::string CPUarch = "";
			std::string CPUmodel = "";
			while (getline(CPUinfoFile,line) && (CPUarch == "" || CPUmodel == ""))
			{
				std::string labelArch = line.substr(0,16);
				std::for_each(labelArch.begin(), labelArch.end(), [](char & c){ c = ::tolower(c); });
				std::string labelPart = labelArch.substr(0,8);
				if (CPUarch == "" && labelArch == "cpu architecture")
				{
					CPUarch = "v"+line.substr(line.find(':')+2);
				}
				else if (CPUmodel == "" && labelPart == "cpu part")
				{
					std::string model = line.substr(line.find(':')+2);
					char* test;
					long model_id = strtol(model.c_str(), NULL, 16);
					switch (model_id)
					{
						case 0xD84:
							CPUmodel = "Neoverse V3";
							break;
						case 0xD8E:
							CPUmodel = "Neoverse N3";
							break;
						case 0xD4F:
							CPUmodel = "Neoverse V2";
							break;
						case 0xD49:
							CPUmodel = "Neoverse N2";
							break;
						case 0xD40:
							CPUmodel = "Neoverse V1";
							break;
						case 0xD0C:
							CPUmodel = "Neoverse N1";
							break;
						default:
							CPUmodel = model;
					}
				}
			}
			CPUname += CPUarch + " " + CPUmodel;
		}
		// else 
		// 0x69 = Intel
		// 0x56 = Marwell
		
		CPUinfoFile.close();
	}
	if (CPUname != "")
	{
		return;
	}

	//internal alternative name of the hardware
	if (!mapThreadsToCore.size())
		initHWSizes();
	CPUname = std::to_string(SOCKETS_PER_NODE)+"x"+std::to_string(CORES_PER_SOCKET)+"x"+std::to_string(THREADS_PER_CORE);
	if (defaultSettings.maxCoreFreq > 0)
		CPUname += " @ "+std::to_string(defaultSettings.maxCoreFreq/1e9)+"GHz";
	else
	{
		CPUinfoFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
		if (CPUinfoFile.is_open())
		{
			CPUinfoFile >> line;
			CPUinfoFile.close();
			CPUname += " @ "+std::to_string(std::stol(line)/1e6)+"GHz";
		}
	}
}

void Environment::getCPUFamilyModelString(std::string &CPUFamilyModelOut)
{
	// NOTE: No consistent CPU family model strings for non-x86 platforms.
	CPUFamilyModelOut = "";
}
#endif


#if defined HAVE_NVML_H || defined HAVE_ROCM_SMI_ROCM_SMI_H
/**
 * Get name of an Nvidia or AMD GPU
 * The name is an alphanumeric string that denotes a particular product.
 * It will not exceed 256 characters in length (including the null terminator).
 **/
void Environment::getGPUbrandString(std::string & GPUname)
{
	const int GPU_NAME_LENGTH = 256;
	char name [GPU_NAME_LENGTH];
	
	#ifdef HAVE_NVML_H
	if (NVML::getGPUsPerNode() && nvmlDeviceGetName (NVML::getGPUHandle(0), name, GPU_NAME_LENGTH) == NVML_SUCCESS)
	#elif defined HAVE_ROCM_SMI_ROCM_SMI_H
	if(rsmi_init(0) == RSMI_STATUS_SUCCESS && rsmi_dev_name_get(0, name, GPU_NAME_LENGTH) == RSMI_STATUS_SUCCESS)
	#endif
	{
		GPUname = std::string(name);
	}
	else
	{
		GPUname = "";
	}
	#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	rsmi_shut_down();
	#endif
}
#endif


/**
 * returns nominal CPU core frequency in Hz
 * if libmsr available uses MSR_PLATFORM_INFO register
 * else if it is x86 architecture uses CPUID instruction
 * else if acpi-cpufreq scaling driver is used returns value from the system file
 * else returns 0
 **/
#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_PLATFORM_INFO
long Environment::getNominalFrequency()
{
	uint64_t MSR_PLATFORM_INFO_bits;
	read_msr(0, MSR_PLATFORM_INFO, &MSR_PLATFORM_INFO_bits);
	return ((MSR_PLATFORM_INFO_bits & MSR_PLATFORM_INFO_nominal_freq_mask) >> MSR_PLATFORM_INFO_nominal_freq_shift) * 100000000;
}
#else
long Environment::getNominalFrequency()
{
	long frequency = 0;
	std::ifstream frequencyBounderyFile;
	std::string freqBound = "";

	struct stat info;
	if(stat("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_nominal_freq", &info) == 0)
	{
		frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_nominal_freq");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
			frequency = std::stol(freqBound)*1000;
		}
	}
	else if(stat("/sys/devices/system/cpu/cpu0/cpufreq/base_frequency", &info) == 0)
	{
		frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/base_frequency");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
			frequency = std::stol(freqBound)*1000;
		}
	}
	else if (getScalingDriver() == "acpi-cpufreq")
	{
		frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
			frequency = std::stol(freqBound);
			if (frequency%10000 == 1000)
			{
				frequency = (frequency-1000) * 1000;
			}
			else
			{
				frequency *= 1000;
			}
		}
	}
	#ifdef _INTEL_
	else
	{
		std::string CPUname;
		getCPUbrandString(CPUname);
		std::size_t pos = CPUname.find('@');
		if (pos != std::string::npos)
		{
			CPUname = CPUname.substr(pos+1);
			frequency = std::stod(CPUname.substr(0,CPUname.size()-3))*1e9;
		}
	}
	#elif defined _AMD_
	else	//AMD CPU P0 frequency
	{
		if (!defaultSettings.availableCoreFreq.empty())
		{
			frequency = defaultSettings.availableCoreFreq[0];
		}
		#if defined HAVE_MSR_SAFE && defined MSR_PSTATE_DEF_P0
		else //else branch cannot call getAvailableCF() because the vector is filled only if esmi not available
		{
			uint64_t MSR_PSTATE_DEF_P0_bits;
			read_msr(0, MSR_PSTATE_DEF_P0, &MSR_PSTATE_DEF_P0_bits);
			if ((MSR_PSTATE_DEF_P0_bits & MSR_PSTATE_DEF_Px_pstate_enable_mask) >> MSR_PSTATE_DEF_Px_pstate_enable_shift)
			{
				double CPUDfsID;
				const uint64_t MSR_PSTATE_DEF_P0_divisor = (MSR_PSTATE_DEF_P0_bits & MSR_PSTATE_DEF_Px_divisor_mask) >> MSR_PSTATE_DEF_Px_divisor_shift;
				switch (MSR_PSTATE_DEF_P0_divisor)
				{
					case 0x8:
						CPUDfsID = 1.0;
						break;
					case 0x9:
						CPUDfsID = 1.125;
						break;
					default:
						CPUDfsID = MSR_PSTATE_DEF_P0_divisor/8.0;
					//`reserved` and `off` values not considered, PstateEnable=0 expected in these cases
				}
				frequency = (((MSR_PSTATE_DEF_P0_bits & MSR_PSTATE_DEF_Px_frequency_id_mask) * 25)/ CPUDfsID)*1e6;
			}
		}
		#endif
	}
	#endif
	return frequency;
}
#endif

/**
 * fills vector of CPU core frequencies [Hz] that can be set
 * ONLY if the list is specified
 * the highest frequency at index 0
 **/
void Environment::getAvailableCF(std::vector<long> & values)
{
	if (!values.empty())
	{
		return;
	}
	std::ifstream frequencyBounderyFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies");
	if (frequencyBounderyFile.is_open())
	{
		std::string freqBound = "";
		while (frequencyBounderyFile >> freqBound)
		{
			long freq = std::stol(freqBound) * 1000;
			values.push_back(freq);
		}
		frequencyBounderyFile.close();
	}
#if defined _AMD_ && !defined(HAVE_E_SMI_E_SMI_H) && HAVE_MSR_SAFE && defined MSR_PSTATE_CUR_LIM && defined MSR_PSTATE_DEF_P0
	else
	{
		uint64_t MSR_PSTATE_CUR_LIM_bits;
		read_msr(0, MSR_PSTATE_CUR_LIM, &MSR_PSTATE_CUR_LIM_bits);
		
		uint64_t MSR_PSTATE_DEF_Px = MSR_PSTATE_DEF_P0;
		const int maxPstate = (MSR_PSTATE_CUR_LIM_bits & MSR_PSTATE_CUR_LIM_max_pstate_mask) >> MSR_PSTATE_CUR_LIM_max_pstate_shift;	
		for (int i=0; i <= maxPstate; i++)
		{
			uint64_t MSR_PSTATE_DEF_Px_bits;
			read_msr(0, MSR_PSTATE_DEF_Px, &MSR_PSTATE_DEF_Px_bits);
			if ((MSR_PSTATE_DEF_Px_bits & MSR_PSTATE_DEF_Px_pstate_enable_mask) >> MSR_PSTATE_DEF_Px_pstate_enable_shift)
			{
				double CPUDfsID;
				const uint64_t MSR_PSTATE_DEF_Px_divisor = (MSR_PSTATE_DEF_Px_bits & MSR_PSTATE_DEF_Px_divisor_mask) >> MSR_PSTATE_DEF_Px_divisor_shift;
				switch (MSR_PSTATE_DEF_Px_divisor)
				{
					case 0x8:
						CPUDfsID = 1.0;
						break;
					case 0x9:
						CPUDfsID = 1.125;
						break;
					default:
						CPUDfsID = MSR_PSTATE_DEF_Px_divisor/8.0;
					//`reserved` and `off` values not considered, PstateEnable=0 expected in these cases
				}
				const uint64_t MSR_PSTATE_DEF_Px_frequency_id = (MSR_PSTATE_DEF_Px_bits & MSR_PSTATE_DEF_Px_frequency_id_mask) * 25;
				values.push_back((MSR_PSTATE_DEF_Px_frequency_id / CPUDfsID)*1e6);
			}
			//else //might be required to match frequency with AMD CPU Pstate
			//	values.push_back(0);
			MSR_PSTATE_DEF_Px++;
		}
	}
#endif
	//return the frequencies ordered, the highest frequency first
	if (values.size() > 1 && values[0] < values[1])
	{
		std::reverse(values.begin(), values.end());
	}
}

/**
 * creates list of available uncore frequencies
 * ! currently not used in MERIC
 * TODO !!! ARM system, that has memory frequency represented as the uncore frequency
 **/
/*
void Environment::getAvailableUCF(std::vector<long> & values)
{
	for (long freq = defaultSettings.minUncoreFreq; freq < defaultSettings.maxUncoreFreq; freq+=100000000)
		values.push_back(freq);
}
*/

#ifdef HAVE_NVML_H
/**
 * reads available CUDA GPU memory and corresponding streaming multiprocessor frequencies
 **/
void Environment::getAvailableGPUfrequencies(std::map<uint, std::vector<uint>> & values)
{
	if (NVML::getGPUsPerNode() == 0)
	{
		return;
	}
	uint memFreqCount = 0;
	uint gpuFreqCount = 0;
	uint *memFreq = NULL;
	uint *gpuFreq = NULL;

	if(nvmlDeviceGetSupportedMemoryClocks(NVML::getGPUHandle(0), &memFreqCount, memFreq) == NVML_ERROR_INSUFFICIENT_SIZE)
	{
		memFreq = (uint *)malloc(memFreqCount * sizeof(uint));
		if(nvmlDeviceGetSupportedMemoryClocks(NVML::getGPUHandle(0), &memFreqCount, memFreq) == NVML_SUCCESS)
		{
			for(uint i = 0; i < memFreqCount; i++)
			{
				if(nvmlDeviceGetSupportedGraphicsClocks(NVML::getGPUHandle(0), memFreq[i], &gpuFreqCount, gpuFreq) == NVML_ERROR_INSUFFICIENT_SIZE)
				{
					gpuFreq = (uint *)malloc(gpuFreqCount * sizeof(uint));
					if(nvmlDeviceGetSupportedGraphicsClocks(NVML::getGPUHandle(0), memFreq[i], &gpuFreqCount, gpuFreq) == NVML_SUCCESS)
					{
						std::vector<uint> SMfreq(gpuFreqCount);
						for (uint j = 0; j<gpuFreqCount; j++)
							SMfreq[j] = gpuFreq[j]*1e6;
						std::reverse(SMfreq.begin(),SMfreq.end());
						values[memFreq[i]*1e6] = SMfreq;
					}
					else
					{
						std::cerr << "MERIC ERROR: NVML cannot read available GPU frequencies\n";
						return;
					}
					free(gpuFreq);
					gpuFreqCount = 0;
				}
				else
				{
					std::cerr << "MERIC ERROR: NVML cannot read available GPU frequencies\n";
					return;
				}
			}
		}
		else
		{
			std::cerr << "MERIC ERROR: NVML cannot read available GPU frequencies\n";
			return;
		}
		free(memFreq);
	}
	else
	{
		std::cerr << "MERIC ERROR: NVML cannot read available GPU frequencies\n";
	}
	return;
}
#endif

#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
/**
 * reads available AMD GPU memory and streaming multiprocessor frequencies
 **/
void Environment::getAvailableGPUfrequencies(std::map<uint, std::vector<uint>> & values)
{
	if (ROCM::getGPUsPerNode() == 0)
	{
		return;
	}
	rsmi_frequencies_t sysFreqs;
	rsmi_frequencies_t memFreqs;

	if(rsmi_dev_gpu_clk_freq_get(0, RSMI_CLK_TYPE_MEM, &memFreqs) == RSMI_STATUS_SUCCESS)
	{
		if(rsmi_dev_gpu_clk_freq_get(0, RSMI_CLK_TYPE_SYS, &sysFreqs) == RSMI_STATUS_SUCCESS)
		{
			//the same thing here - incompatible uints
			std::vector<uint> SMfreq(sysFreqs.num_supported);
			for(int i = 0; i < sysFreqs.num_supported; i++)
			{
				SMfreq[i] = sysFreqs.frequency[i];
			}
			std::reverse(SMfreq.begin(),SMfreq.end());
			for(int i = 0; i < memFreqs.num_supported; i++)
			{
				values[memFreqs.frequency[i]] = SMfreq;
			}
		}
		else
		{
			std::cerr << "MERIC ERROR: RSMI cannot read available GPU frequencies\n";
		}
	}
	else
	{
		std::cerr << "MERIC ERROR: RSMI cannot read available GPU frequencies\n";
	}
}
#endif


/**
 * returns maximum available non-AVX turbo CPU core frequency in Hz
 * when the information is not available, returns 0
 * does not take into consideration whether the turbo frequencies are enabled
 * note: AVX/AVX2 and AVX512 frequency limits of Intel CPUs provided by systemInfo tool only, other vendors not supported
 **/
long Environment::getMaximumTurboFrequency()
{
	long frequency = 0;
#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_TURBO_RATIO_LIMIT
	uint64_t MSR_TURBO_RATIO_LIMIT_bits = 0x0;
	read_msr(0, MSR_TURBO_RATIO_LIMIT, &MSR_TURBO_RATIO_LIMIT_bits);
	if (MSR_TURBO_RATIO_LIMIT_bits != 0x0)
	{
		MSR_TURBO_RATIO_LIMIT_bits = MSR_TURBO_RATIO_LIMIT_bits & MSR_TURBO_RATIO_LIMIT_mask;
		frequency = (MSR_TURBO_RATIO_LIMIT_bits & MSR_TURBO_RATIO_LIMIT_mask) * 100000000;
	}
#elif defined HAVE_E_SMI_E_SMI_H
	uint32_t boostlimit;
	if (!esmi_core_boostlimit_get(0, &boostlimit))
	{
		frequency = boostlimit * 1e6;
	}
#endif

	if (frequency == 0)
	{
		struct stat info;
		if(stat("/sys/devices/system/cpu/cpu0/cpufreq/scaling_boost_frequencies", &info) == 0)
		{
			std::ifstream frequencyBounderyFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_boost_frequencies");
			std::vector<long> values;
			if (frequencyBounderyFile.is_open())
			{
				std::string freqBound = "";
				while (frequencyBounderyFile >> freqBound)
				{
					long freq = std::stol(freqBound) * 1000;
					values.push_back(freq);
				}
				frequencyBounderyFile.close();

				if (values.size() > 2 && values[0] < values[1])
				{
					frequency = values.back();
				}
				else if (values.size())
				{
					frequency = values[0];
				}
			}
		}
		else
		{
			std::ifstream frequencyBounderyFile("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
			if (frequencyBounderyFile.is_open())
			{
				std::string freqBound;
				frequencyBounderyFile >> freqBound;
				frequencyBounderyFile.close();
				frequency = std::stol(freqBound) * 1000;
				if (frequency-1000000 == getNominalFrequency())
				{
					frequency = 0;
				}
			}
		}
	}

	return frequency;
}

/**
 * returns current scaling driver
 **/
std::string Environment::getScalingDriver()
{
	if (scalingDriver == "")
	{
		std::ifstream frequencyBounderyFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_driver");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> scalingDriver;
			frequencyBounderyFile.close();
		}
	}
	return scalingDriver;
}

/**
 * Discovers if the Hardware controlled P-State is active
 * returns true if HWP is active
 * the status std::string is set according the status of the HWP
 * enabled / disabled / not available
 **/
#ifdef _INTEL_
bool Environment::isHWPactive(std::string &status)
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
		#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined IA32_PM_ENABLE
			uint64_t IA32_PM_ENABLE_bits;
			read_msr(0, IA32_PM_ENABLE, &IA32_PM_ENABLE_bits);
			HWP = IA32_PM_ENABLE_bits & IA32_PM_ENABLE_mask;
			status = HWP ? "enabled" : "disabled";
		#else
			status = "available, status unknown";
		#endif
	}
	else
	{
		status = "not available";
	}
	return HWP;
}
#endif

/**
 * tests if the CPU core frequency scaling is possible
 * returns value from DVFS_WAY which give us information how the frequency can be scaled
 **/
DVFS_WAY Environment::isDVFSpossible()
{
#ifdef _INTEL_
	std::string status;
	if (isHWPactive(status))
	{
		if (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", W_OK) == 0 && access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq", W_OK) == 0)
		{
			return HWP;
		}
		else
		{
			return NO_DVFS;
		}
	}
#elif defined _AMD_ 
	#if ( defined HAVE_MSR_SAFE && defined MSR_HWCR && defined MSR_PSTATE_CTL ) || defined HAVE_E_SMI_E_SMI_H
		//has not been tested for scaling driver requirement TODO !!!
		return OTHER;
	#endif
#endif

#if defined _INTEL_
	#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined IA32_ENERGY_PERF_BIAS && defined IA32_PERF_CTL
	if (getScalingDriver() == "intel_pstate" &&
	    access("/sys/devices/system/cpu/intel_pstate/no_turbo", W_OK) == 0 &&
	    access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", W_OK) == 0)
	{
		return INT_PSTATE; //requires also IA32_ENERGY_PERF_BIAS register in the msr_safe whitelist (allowlist)
	}
	#endif
	if (getScalingDriver() == "intel_cpufreq")
	{
		#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined IA32_PERF_CTL
		return INT_CPUFREQ;
		#else
		if (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", W_OK) == 0)
		{
			return INT_CPUFREQ;
		}
		else
		{
			return NO_DVFS;
		}
		#endif
	}
#endif

	// scalingGovernor is not set if neither readSystemDefaultSettings() nor readRestoreFile() have been called
	// may happen if used from "outside" or if the restoreFile is empty
	if (scalingGovernor == "")
	{
		std::ifstream frequencyBounderyFile;
		frequencyBounderyFile.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> scalingGovernor;
			frequencyBounderyFile.close();
		}
	}

	if (scalingGovernor == "userspace")
	{
		if (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", W_OK) == 0)
		{
#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_X86_ADAPT_H || defined HAVE_CPUFREQ_H || defined HAVE_GEOPM_H
			return USRSPACE_LIB;
#else
			return USRSPACE_FILE;
#endif
		}
		else
		{
			return NO_DVFS;
		}
	}
	
	if (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", W_OK) == 0)
	{
		std::ifstream frequencyBounderyFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors");
		std::string availGovernor;
		while (frequencyBounderyFile >> availGovernor)
		{
			if (availGovernor == "userspace")
			{
				frequencyBounderyFile.close();
				if (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", W_OK) == 0)
				{
#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_X86_ADAPT_H || defined HAVE_CPUFREQ_H || defined HAVE_GEOPM_H
					return USRSPACE_LIB;
#else
					return USRSPACE_FILE;
#endif
				}
				else
				{
					return NO_DVFS;
				}
			}
		}
		frequencyBounderyFile.close();
	}

	return NO_DVFS;
}

/**
 * activates frequency scaling on the system according the DVFS_WAY value
 * if the input DVFS_WAY is unknown (NO_DVFS), it gets the information
 * returns NO_DVFS if scaling not possible otherwise the way it used to activate frequency scaling
 **/
DVFS_WAY Environment::activateDVFS(DVFS_WAY way)
{
	if (way == NO_DVFS)
	{
		way = isDVFSpossible();
	}

	if (way == USRSPACE_FILE || way == USRSPACE_LIB)
	{
		if (scalingGovernor != "userspace")
		{
			std::ofstream governorFileO;
			for (int soc=0; soc<SOCKETS_PER_NODE; soc++)
			{
				for (int cpu : mapCoresToSocket[soc])
				{
					for (int thread : mapThreadsToCore[cpu])
					{
						governorFileO.open ("/sys/devices/system/cpu/cpu"+std::to_string(thread)+"/cpufreq/scaling_governor");
						if (governorFileO.is_open())
						{
							governorFileO << "userspace";
							governorFileO.close();
						}
						if (!governorFileO)
						{
							std::cerr << "MERIC ERROR: Failed to write to /sys/devices/system/cpu/cpu"<<thread<<"/cpufreq/scaling_governor\n";
							return NO_DVFS;
						}
					}
				}
			}
		}
	}
	else if (way == INT_PSTATE)
	{
		//Disable turbo logic of Intel P-state driver
		std::ofstream defSet;
		defSet.open ("/sys/devices/system/cpu/intel_pstate/no_turbo");
		if (defSet.is_open())
		{
			defSet << "1";
			defSet.close();
		}
		if (!defSet)
		{
			std::cerr << "MERIC ERROR: Failed to write to /sys/devices/system/cpu/intel_pstate/no_turbo\n";
			return NO_DVFS;
		}
	
		//Disable scaling frequency of Intel P-state driver 
		//and set maximum P-state to all cores 
		//and set energy performance bias to maximum performance
		for (int soc=0; soc<SOCKETS_PER_NODE; soc++)
		{
			for (int cpu : mapCoresToSocket[soc])
			{
				for (int thread : mapThreadsToCore[cpu])
				{
					defSet.open ("/sys/devices/system/cpu/cpu"+std::to_string(thread)+"/cpufreq/scaling_max_freq");
					if (defSet.is_open())
					{
						defSet << defaultSettings.minCoreFreq / 1000; //kHz
						defSet.close();
					}
					if (!defSet)
					{
						std::cerr << "MERIC ERROR: Failed to write to /sys/devices/system/cpu/cpu"<<thread<<"/cpufreq/scaling_max_freq\n";
						return NO_DVFS;
					}
				}			
			}
		}
#if defined _INTEL_ && (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined IA32_ENERGY_PERF_BIAS
		for (int soc=0; soc<SOCKETS_PER_NODE; soc++)
		{
			for (int cpu : mapCoresToSocket[soc])
			{
				for (int thread : mapThreadsToCore[cpu])
				{
					write_msr(thread, IA32_ENERGY_PERF_BIAS, 0);
				}
			}
		}
#endif
		//p_state may still rewrite the IA32_PERF_CTL register during a short time period, so we wait
		usleep(800000); //700 ms is not enough
	}
	
	return way;
}

UFS_WAY Environment::isUFSPossible()
{
	UFS_WAY way = getPreferredUFSWay();

	if(way == UFS_WAY_TPMI_SYSFS)
	{
		const std::string basePath = "/sys/devices/system/cpu/intel_uncore_frequency/package_00_die_00/";
		
		if(!sysfsutils::CheckFileAccess({
				{{"min_freq_khz", W_OK}}, {{"max_freq_khz", W_OK}}, 
				{{"initial_min_freq_khz", R_OK}}, {{"initial_max_freq_khz", R_OK}} 
			}, basePath))
		{
			way = UFS_WAY_NONE;
		}
	}

	return way;
}

UFS_WAY Environment::getPreferredUFSWay()
{
#ifdef _INTEL_
	std::string cpuFamilyStr;
	getCPUFamilyModelString(cpuFamilyStr);
	auto cpuFamily = meric::stringToCpuidSignature(cpuFamilyStr);

	if(cpuFamily >= CpuidSignatureValues::INTEL_06_AEH)
	{
		// Granite Rapids and newer Intel CPUs require TPMI interface for tuning
		return UFS_WAY_TPMI_SYSFS;
	}
	else
	{
		// Use MSR for older Intel CPUs or attempt to fallback to TPMI when MSR is not available at compile time
	#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_X86_ADAPT_H || defined HAVE_GEOPM_H
		return UFS_WAY_MSR;
	#else
		return UFS_WAY_TPMI_SYSFS;
	#endif
	}
#elif defined __arm__
	if(systemType == SYS_JETSON)
	{
		return UFS_WAY_JETSON_SYSFS;
	}
#endif

	return UFS_WAY_NONE;
}

/**
 * identifies CUDA driver version, if it is 465.xxx or newer then MERIC will use
 * system utility nvidia-smi calls, for which we expect raised rights (/etc/sudoers)
 * drivers prior 465.xxx had a posibility to raise the rights via `sudo nvidia-smi -acp UNRESTRICTED`
 **/
#ifdef HAVE_NVML_H
void Environment::getGPUTuningMethod()
{
	if (activatedGPUtuning != NO_TUNING)
	{
		return;
	}
	
	if (geteuid() == 0)
	{
		activatedGPUtuning = NVML_API;
		return;
	}
	
	char driverVersion[80];
	if (nvmlSystemGetDriverVersion(driverVersion, 80) == NVML_SUCCESS)
	{
		std::string name = std::string(driverVersion);
		std::string::size_type pos = name.find('.');
		if (pos != std::string::npos && std::stoi(name.substr(0, pos)) > 465)
		{
			activatedGPUtuning = NVIDIA_SMI;
			return;
		}
	}
	activatedGPUtuning = NVML_API;
}
#endif

#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
/**
 * AMD GPU frequency scaling switched to manual mode
 **/
void Environment::getGPUTuningMethod()
{
	if (activatedGPUtuning != NO_TUNING)
	{
		return;
	}
	
	if (geteuid() == 0)
	{
		activatedGPUtuning = ROCM_SMI_API;
		for(int id = 0; id < ROCM::getGPUsPerNode(); id++)
		{
			if(rsmi_dev_perf_level_set(id, RSMI_DEV_PERF_LEVEL_MANUAL) != RSMI_STATUS_SUCCESS)
			{
				std::cerr << "MERIC ERROR: RSMI failed to enable GPU frequency tunning\n";
				activatedGPUtuning = NO_TUNING;
				return;
			}
		}
	}
	else
	{
		system("sudo rocm-smi --setperflevel manual 2> /dev/null > /dev/null \n");
		activatedGPUtuning = ROCM_SMI_UTIL;
	}
	return;
}
#endif

/**
 * return true if power limit modification is possible
 **/
bool Environment::isPWRCAPpossible()
{
#if defined _INTEL_
	// PKG
	uint64_t MSR_PKG_POWER_LIMIT_bits = 0x0;
	
	#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_PKG_POWER_LIMIT
		read_msr(0, MSR_PKG_POWER_LIMIT, &MSR_PKG_POWER_LIMIT_bits);
	#elif defined HAVE_X86_ADAPT_H
		int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
		int adaptRegisterHandler;
		adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_1");
		x86_adapt_get_setting(fd, adaptRegisterHandler, &MSR_PKG_POWER_LIMIT_bits);
	#elif defined HAVE_GEOPM_H
		MSR_PKG_POWER_LIMIT_bits = geopm_signal_to_field(pio.read_signal("MSR::PKG_POWER_LIMIT#", pio.signal_domain_type("MSR::PKG_POWER_LIMIT#"), 0)) & 0xFFFFFFFF;
	#endif
	
	if (MSR_PKG_POWER_LIMIT_bits != 0x0 && (MSR_PKG_POWER_LIMIT_bits & MSR_PKG_POWER_LIMIT_lock_mask) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
#elif defined _AMD_ //remaining CPU architectures with supported power cap limit tuning
	return true;
#else
	return false;
#endif
}

/**
 * parse a list of threads IDs (comma or dash seprarated list) and inserts the ids to the vector vec
 * if lastOnly is true, only id of the very last element of the list is inserted to the vector vec
 * due to avalability of the lastOnly parameter the vecor is in opposite order
 **/
void Environment::stringOfIDsToVector(std::string list, std::vector<int> &vec, bool lastOnly)
{
	auto parseRange = [&] (std::string range)
	{
		size_t pos = range.find('-');
		if (pos != std::string::npos)
		{
			int start = std::stoi(range.substr(0,pos));
			int stop  = std::stoi(range.substr(pos+1));
			do
			{
				vec.push_back(stop);
				stop--;
			}while(stop >= start && lastOnly != true);
		}
		else
			vec.push_back(std::stoi(range));
	};
	
	std::size_t position;
	do
	{
		position = list.find_last_of(',');
		if (position != std::string::npos)
		{
			parseRange(list.substr(position+1));
			list = list.substr(0,position);
		}
		else
			parseRange(list);
	}while(position != std::string::npos && lastOnly != true);
}

/**
 * detects the system topology
 * 	- list of threads of a core
 * 	- list of cores of a socket
 * this method sets following variables - system sizes:
 *  - SOCKETS_PER_NODE
 *  - CORES_PER_SOCKET
 *  - THREADS_PER_CORE
 **/
void Environment::initHWSizes()
{
	std::string topoStr = "";
	std::ifstream topoFile;
	
	topoFile.open("/sys/devices/system/cpu/online");
	if (topoFile.is_open())
	{
		topoFile >> topoStr;
		topoFile.close();
	}
	std::vector<int> onlineThreads;
	stringOfIDsToVector(topoStr, onlineThreads);
	
	while(onlineThreads.size())
	{
		topoStr = "";
		topoFile.open("/sys/devices/system/cpu/cpu"+std::to_string(onlineThreads[0])+"/topology/thread_siblings_list");
		if (topoFile.is_open())
		{
			topoFile >> topoStr;
			topoFile.close();
		}
		std::vector<int> threadSiblings;
		stringOfIDsToVector(topoStr, threadSiblings);
		mapThreadsToCore[threadSiblings.back()] = threadSiblings;
		
		if (threadSiblings.size() > THREADS_PER_CORE)
		{
			THREADS_PER_CORE = threadSiblings.size();
		}
		for(int thrd : threadSiblings)
		{
			onlineThreads.erase(std::remove(onlineThreads.begin(), onlineThreads.end(), thrd), onlineThreads.end());
		}
	}
	
	// /sys/devices/system/cpu/cpu * /topology/physical_package_id
	// physical package id of cpuX. Typically corresponds to a physical socket number, 
	// but the actual value is architecture and platform dependent.

	// converting the IDs to numbering starting from zero
	std::map <unsigned int, unsigned int> package2socket;
	
	for (const auto &core : mapThreadsToCore)
	{
		topoStr = "";
		topoFile.open("/sys/devices/system/cpu/cpu"+std::to_string(core.first)+"/topology/physical_package_id");
		if (topoFile.is_open())
		{
			topoFile >> topoStr;
			topoFile.close();
		}
		
		int soc = std::stoi(topoStr);
		if (package2socket.find(soc) == package2socket.end())
		{
			package2socket[soc] = package2socket.size();
		}
		
		soc = package2socket[soc];
		
		if (mapCoresToSocket.count(soc))
		{
			mapCoresToSocket[soc].push_back(core.first);
			if (CORES_PER_SOCKET < mapCoresToSocket[soc].size())
			{
				CORES_PER_SOCKET++;
			}
		}
		else
		{
			mapCoresToSocket[soc] = {core.first};
		}
	}
	SOCKETS_PER_NODE = mapCoresToSocket.size();
	
	MERIC_INFO << "# SYSTEM INFORMATION\n"
	           << "\tSockets per Node: " << SOCKETS_PER_NODE << std::endl
	           << "\tCores per Socket: " << CORES_PER_SOCKET << std::endl
	           << "\tThreads per Core: " << THREADS_PER_CORE << std::endl;

	/*
	MERIC_INFO << "# TOPOLOGY: \n";
	for (const auto &socket : mapCoresToSocket)
	{
		MERIC_INFO << "\tSOC " << socket.first << ": ";
		for (int core : socket.second)
		{
			MERIC_INFO << core << " ";
			if (mapThreadsToCore[core].size() > 1)
			{
				MERIC_INFO << "( ";
				for (int thread: mapThreadsToCore[core])
					MERIC_INFO << thread << " ";
				MERIC_INFO << ") ";
			}
		}
		MERIC_INFO << std::endl;
	}
	*/
}

/**
 * reads a file with the default setting of the system that MERIC stored during 
 * a previous run
 * parameter restoreJson will be filled with the content of the read file
 * modifTime holds time of the last modification, a modified file will not be re-written
 * returns true if the restore file exists and was read, otherwise false
 **/
bool Environment::readRestoreFile(std::string & restoreJson, timespec & modifTime)
{
	restoreJson = "";
	std::string defaultSettingFile = std::string(MERIC_TMP_PATH)+"/systemDefault.json";
	
	struct stat info;
	if(stat(defaultSettingFile.c_str(), &info) == 0)
	{
		modifTime = info.st_mtim;
		std::ifstream jsonFile (defaultSettingFile);
		std::string line = "";
		if (jsonFile.is_open())
		{
			while (getline(jsonFile,line))
			{
				restoreJson += " " + line+"\n";
			}
			jsonFile.close();
		}
		if (restoreJson == "")
		{
			std::cerr << "MERIC WARNING: empty restore file, creating a new one\n";
			rename(defaultSettingFile.c_str(), (defaultSettingFile+".err").c_str());
			return false;
		}
		
		const char * json = restoreJson.c_str();
		struct json_value_s* root = json_parse(json, strlen(json));
		if (root == NULL)
		{
			std::cerr << "MERIC ERROR: invalid " << defaultSettingFile << std::endl;
			rename(defaultSettingFile.c_str(), (defaultSettingFile+".err").c_str());
			restoreJson = "";
			return false;
		}
		
		//expecting that all CPUs and all GPUs are the same kind
		std::string CPUname;
		getCPUbrandString(CPUname);
#if defined HAVE_NVML_H || defined HAVE_ROCM_SMI_ROCM_SMI_H
		std::string GPUname;
		getGPUbrandString(GPUname);
		int HWperNode = 2;
#else
		int HWperNode = 1;
#endif
		struct json_object_element_s* data = ((struct json_object_s*)root->payload)->start;
		while(data != NULL)
		{
			if (std::string(data->name->string) == CPUname)
			{
				HWperNode--;
				struct json_object_element_s* architectureData = ((struct json_object_s*)data->value->payload)->start;
				while(architectureData != NULL)
				{
					if (std::string(architectureData->name->string) == "CF")
					{
						struct json_object_element_s* dvfs = ((struct json_object_s*)architectureData->value->payload)->start;
						do
						{
							if (std::string(dvfs->name->string) == "GOVERNOR")
							{
								scalingGovernor = std::string(((struct json_string_s*)dvfs->value->payload)->string);
							}
							else if (std::string(dvfs->name->string) == "TURBO")
							{
								defaultSettings.maxTurboFreq = std::stol(std::string(((struct json_string_s*)dvfs->value->payload)->string));
							}
							else if (std::string(dvfs->name->string) == "MAX")
							{
								defaultSettings.maxCoreFreq = std::stol(std::string(((struct json_string_s*)dvfs->value->payload)->string));
							}
							else if (std::string(dvfs->name->string) == "MIN")
							{
								defaultSettings.minCoreFreq = std::stol(std::string(((struct json_string_s*)dvfs->value->payload)->string));
							}
							else if (std::string(dvfs->name->string) == "NOMINAL")
							{
								defaultSettings.nominalCoreFreq = std::strtoull(std::string(((struct json_string_s*)dvfs->value->payload)->string).c_str(), NULL, 0);
							}
							else if (std::string(dvfs->name->string) == "DEFAULT")
							{
								defaultSettings.defaultCoreFreq = std::stol(std::string(((struct json_string_s*)dvfs->value->payload)->string));
							}
#ifdef _INTEL_
							else if (std::string(dvfs->name->string) == "ENERGY_PERF_BIAS")
							{
								defaultSettings.IA32_ENERGY_PERF_BIAS_default = std::strtoull(std::string(((struct json_string_s*)dvfs->value->payload)->string).c_str(), NULL, 0);
							}
#elif defined _AMD_
							else if (std::string(dvfs->name->string) == "HWCR")
							{
								defaultSettings.MSR_HWCR_default = std::strtoull(std::string(((struct json_string_s*)dvfs->value->payload)->string).c_str(), NULL, 0);
							}
#endif
							else if (std::string(dvfs->name->string) == "AVAILABLE")
							{
#if !defined HAVE_E_SMI_E_SMI_H
								//the AMD CPU may have defined P-State frequencies, however esmi allows to set any frequency, so we ignore the list
								struct json_array_element_s* CFlist = ((struct json_array_s*)dvfs->value->payload)->start;
								while(CFlist != NULL)
								{
									defaultSettings.availableCoreFreq.push_back(std::stol(((struct json_string_s*)CFlist->value->payload)->string));
									CFlist = CFlist->next;
								}							
#endif
							}
							dvfs = dvfs->next;
						}while(dvfs != NULL);
					}
					else if (std::string(architectureData->name->string) == "UFS")
					{
						struct json_object_element_s* ufs = ((struct json_object_s*)architectureData->value->payload)->start;
						do
						{
							if (std::string(ufs->name->string) == "MAX")
							{
								defaultSettings.maxUncoreFreq = std::stol(std::string(((struct json_string_s*)ufs->value->payload)->string));
							}
							else if (std::string(ufs->name->string) == "MIN")
							{
								defaultSettings.minUncoreFreq = std::stol(std::string(((struct json_string_s*)ufs->value->payload)->string));
							}
							else if (std::string(ufs->name->string) == "DEFAULT")
							{
								defaultSettings.defaultUncoreFreq = std::stol(std::string(((struct json_string_s*)ufs->value->payload)->string));
							}
							ufs = ufs->next;
						}while(ufs != NULL);
					}
#if defined _INTEL_ || defined _AMD_
					else if (std::string(architectureData->name->string) == "PWRCAP")
					{
						struct json_object_element_s* pwr = ((struct json_object_s*)architectureData->value->payload)->start;
						do
						{
							if (std::string(pwr->name->string) == "PKG")
							{
								defaultSettings.defaultPowerCapPKG = std::strtoull(std::string(((struct json_string_s*)pwr->value->payload)->string).c_str(), NULL, 0);
							}
							#if defined _INTEL_
							else if (std::string(pwr->name->string) == "DRAM")
							{
								defaultSettings.defaultPowerCapRAM = std::strtoull(std::string(((struct json_string_s*)pwr->value->payload)->string).c_str(), NULL, 0);
							}
							#endif
							pwr = pwr->next;
						}while(pwr != NULL);
					}
#endif
					architectureData = architectureData->next;
				}
			}
#if defined HAVE_NVML_H || defined HAVE_ROCM_SMI_ROCM_SMI_H
			else if (std::string(data->name->string) == GPUname)
			{
				HWperNode--;
				struct json_object_element_s* architectureData = ((struct json_object_s*)data->value->payload)->start;
				while(architectureData != NULL)
				{
					if (std::string(architectureData->name->string) == "FREQ")
					{
						struct json_object_element_s* dvfs = ((struct json_object_s*)architectureData->value->payload)->start;
						do
						{
							if (std::string(dvfs->name->string) == "DEFAULT_MEMORY")
							{
								defaultSettings.defaultGPUMemFreq = std::strtoull(std::string(((struct json_string_s*)dvfs->value->payload)->string).c_str(), NULL, 0);
							}
							else if (std::string(dvfs->name->string) == "DEFAULT_MULTIPROCESSOR")
							{
								defaultSettings.defaultGPUSMFreq = std::strtoull(std::string(((struct json_string_s*)dvfs->value->payload)->string).c_str(), NULL, 0);
							}
							else if (std::string(dvfs->name->string) == "AVAILABLE")
							{
								struct json_object_element_s* freqlist = ((struct json_object_s*)dvfs->value->payload)->start;
								while(freqlist != NULL)
								{
									std::vector <uint> SMfreq;
									struct json_array_element_s* SMlist = ((struct json_array_s*)freqlist->value->payload)->start;
									do
									{
										SMfreq.push_back(std::stoull(((struct json_string_s*)SMlist->value->payload)->string));
										SMlist = SMlist->next;
									}while(SMlist != NULL);
									defaultSettings.availableGPUFreq[std::stoull(freqlist->name->string)] = SMfreq;
									freqlist = freqlist->next;
								}
							}
							dvfs = dvfs->next;
						}while(dvfs != NULL);

					}
					else if (std::string(architectureData->name->string) == "PWRCAP")
					{
						struct json_object_element_s* pwr = ((struct json_object_s*)architectureData->value->payload)->start;
						do
						{
							if (std::string(pwr->name->string) == "DEFAULT")
							{
								defaultSettings.defaultPowerCapGPU = std::stoull(std::string(((struct json_string_s*)pwr->value->payload)->string));
							}
							else if (std::string(pwr->name->string) == "MIN")
							{
								defaultSettings.minPowerCapGPU = std::stoull(std::string(((struct json_string_s*)pwr->value->payload)->string));
							}
							else if (std::string(pwr->name->string) == "MAX")
							{
								defaultSettings.maxPowerCapGPU = std::stoull(std::string(((struct json_string_s*)pwr->value->payload)->string));
							}
							pwr = pwr->next;
						}while(pwr != NULL);
					}
					architectureData = architectureData->next;
				}
			}
#endif
			//information about HW on node loaded from the json
			if (HWperNode == 0)
			{
				free(root);
				return true;
			}
			data = data->next;
		}
		free(root);
		return false;
	}

	return false;
}

/**
 * reads current system settings, which is considered to be default, despite the 
 * system may provide different values to be default
 * if available, reads the settings from a json file, that should provide the default settings before we have started any tuning tests
 * if the configuration not available in the json, than updates the file
 **/
void Environment::readSystemDefaultSettings()
{
	timespec modifTime;
	std::string restoreJson;
	struct stat info;
	if (!readRestoreFile(restoreJson, modifTime))
	{
	//if the json file does not exists, does not contain current architecture or is corrupted
	//we have to read the default settings from the system

// CPU CORE freq -------------------------------------------------------------//
		defaultSettings.defaultCoreFreq = 0;
		std::ifstream frequencyBounderyFile;
		frequencyBounderyFile.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> scalingGovernor;
			frequencyBounderyFile.close();
		}
		std::string freqBound; //files have the values in KHz
		frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
			defaultSettings.maxCoreFreq = std::stol(freqBound) * 1000;
		}

		if (stat("/sys/devices/system/cpu/intel_pstate/no_turbo", &info) == 0)
		{
			frequencyBounderyFile.open ("/sys/devices/system/cpu/intel_pstate/no_turbo");
			if (frequencyBounderyFile.is_open())
			{
				frequencyBounderyFile >> freqBound;
				frequencyBounderyFile.close();
				if (std::stoi(freqBound) == 1)
					defaultSettings.maxTurboFreq = 0;
				else
				{
					defaultSettings.maxTurboFreq = getMaximumTurboFrequency();
					if (defaultSettings.maxTurboFreq == 0)
						defaultSettings.maxTurboFreq = 1;
				}
			}
		}
		else if (stat("/sys/devices/system/cpu/cpufreq/boost", &info) == 0)
		{
			frequencyBounderyFile.open ("/sys/devices/system/cpu/cpufreq/boost");
			if (frequencyBounderyFile.is_open())
			{
				frequencyBounderyFile >> freqBound;
				frequencyBounderyFile.close();
				if (std::stoi(freqBound) == 1)
				{
					defaultSettings.maxTurboFreq = getMaximumTurboFrequency();
					if (defaultSettings.maxTurboFreq == 0)
						defaultSettings.maxTurboFreq = 1;
				}
			}
		}
#if defined _AMD_
		#if defined HAVE_E_SMI_E_SMI_H
		else
		{
			defaultSettings.maxTurboFreq = getMaximumTurboFrequency();
			if (defaultSettings.maxTurboFreq == 0)
				defaultSettings.maxTurboFreq = 1;
		}
		#elif defined HAVE_MSR_SAFE && defined MSR_HWCR
		else
		{
			uint64_t MSR_HWCR_bits = 0x0;
			read_msr(0, MSR_HWCR, &MSR_HWCR_bits);
			defaultSettings.maxTurboFreq = (MSR_HWCR_bits & MSR_HWCR_boost_disable_mask) >> MSR_HWCR_boost_disable_shift ? 0 : 1;
		}
		#endif
#endif


#if !defined HAVE_E_SMI_E_SMI_H
		//the AMD CPU may have defined P-State frequencies, however it is not limiting when using esmi
		getAvailableCF(defaultSettings.availableCoreFreq);
#endif

		if (scalingGovernor == "userspace")
		{
			frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
			if (frequencyBounderyFile.is_open())
			{
				frequencyBounderyFile >> freqBound;
				frequencyBounderyFile.close();
				defaultSettings.defaultCoreFreq = std::stol(freqBound) * 1000;
			}
		}
#if defined _AMD_ && !defined HAVE_E_SMI_E_SMI_H && defined HAVE_MSR_SAFE && defined MSR_PSTATE_CUR_LIM
		// if using esmi, when restoring default CF we will set turbo frequency
		else if (!defaultSettings.availableCoreFreq.empty())
		{
			uint64_t MSR_PSTATE_CTL_bits;
			read_msr(0, MSR_PSTATE_CTL, &MSR_PSTATE_CTL_bits);
			defaultSettings.defaultCoreFreq = defaultSettings.availableCoreFreq[MSR_PSTATE_CTL_bits & MSR_PSTATE_CUR_LIM_curr_limit_pstate_mask];
			if (defaultSettings.maxTurboFreq > 0)
				defaultSettings.defaultCoreFreq += 1000000; 
		}
#endif
	
		frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
			defaultSettings.minCoreFreq = std::stol(freqBound) * 1000;
		}

		frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
			defaultSettings.minCoreFreq = std::stol(freqBound) * 1000;
		}
		
		defaultSettings.nominalCoreFreq = getNominalFrequency();

#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H
	#if defined _INTEL_ && defined IA32_ENERGY_PERF_BIAS
		read_msr(0, IA32_ENERGY_PERF_BIAS, &defaultSettings.IA32_ENERGY_PERF_BIAS_default);
	#elif defined _AMD_ && defined MSR_HWCR
		read_msr(0, MSR_HWCR, &defaultSettings.MSR_HWCR_default);
	#endif
#endif

// CPU UnCORE freq -----------------------------------------------------------//
		defaultSettings.defaultUncoreFreq = 0;
#ifdef _INTEL_
		if(getPreferredUFSWay() == UFS_WAY_TPMI_SYSFS)
		{
			frequencyBounderyFile.open("/sys/devices/system/cpu/intel_uncore_frequency/package_00_die_00/initial_min_freq_khz");
			if(frequencyBounderyFile.is_open())
			{
				frequencyBounderyFile >> freqBound;
				frequencyBounderyFile.close();
				defaultSettings.minUncoreFreq = std::stol(freqBound) * 1000;
			}

			frequencyBounderyFile.open("/sys/devices/system/cpu/intel_uncore_frequency/package_00_die_00/initial_max_freq_khz");
			if(frequencyBounderyFile.is_open())
			{
				frequencyBounderyFile >> freqBound;
				frequencyBounderyFile.close();
				defaultSettings.maxUncoreFreq = std::stol(freqBound) * 1000;
			}
		}
		else if(getPreferredUFSWay() == UFS_WAY_MSR)
		{
	#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H
			uint64_t MSR_UNCORE_RATIO_LIMIT_bits;
			read_msr(0, MSR_UNCORE_RATIO_LIMIT, &MSR_UNCORE_RATIO_LIMIT_bits);

			defaultSettings.maxUncoreFreq = (MSR_UNCORE_RATIO_LIMIT_bits & MSR_UNCORE_RATIO_LIMIT_max_mask) * 100000000;
			defaultSettings.minUncoreFreq = ((MSR_UNCORE_RATIO_LIMIT_bits & MSR_UNCORE_RATIO_LIMIT_min_mask) >> MSR_UNCORE_RATIO_LIMIT_min_shift) * 100000000;
	#elif defined HAVE_X86_ADAPT_H
			int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
			int adaptHandlerUncore = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO");
			uint64_t setting;
			x86_adapt_get_setting(fd, adaptHandlerUncore, &setting);
			defaultSettings.maxUncoreFreq = setting * 100000000;
			adaptHandlerUncore = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MIN_RATIO");
			x86_adapt_get_setting(fd, adaptHandlerUncore, &setting);
			defaultSettings.minUncoreFreq = setting * 100000000;
	#elif defined HAVE_GEOPM_H
			defaultSettings.minUncoreFreq = pio.read_signal("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", pio.signal_domain_type("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO"), 0);
			
			defaultSettings.maxUncoreFreq = pio.read_signal("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", pio.signal_domain_type("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO"), 0);
	#endif
		}
#elif defined __arm__	//Jetson memory frequencies
		frequencyBounderyFile.open ("/sys/kernel/debug/clock/emc/max");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
			defaultSettings.maxUncoreFreq = std::stol(freqBound);
		}

		frequencyBounderyFile.open ("/sys/kernel/debug/clock/emc/min");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
			defaultSettings.minUncoreFreq = std::stol(freqBound);
		}
		
		frequencyBounderyFile.open ("/sys/kernel/debug/clock/emc/rate");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
			defaultSettings.defaultUncoreFreq = std::stol(freqBound);
		}
#endif

// CPU POWERcap --------------------------------------------------------------//
#ifdef _INTEL
		uint64_t MSR_PKG_POWER_LIMIT_bits = 0x0;
		uint64_t MSR_DRAM_POWER_LIMIT_bits = 0x0;
	#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H
		#ifdef MSR_PKG_POWER_LIMIT
		read_msr(0, MSR_PKG_POWER_LIMIT, &MSR_PKG_POWER_LIMIT_bits);
		#endif
		#ifdef MSR_DRAM_POWER_LIMIT
		read_msr(0, MSR_DRAM_POWER_LIMIT, &MSR_DRAM_POWER_LIMIT_bits);
		#endif
	#elif defined HAVE_X86_ADAPT_H
		int adaptRegisterHandler;
		uint64_t MSR_PKG_1_POWER_LIMIT_bits, MSR_PKG_2_POWER_LIMIT_bits;
		
		adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_1");
		x86_adapt_get_setting(fd, adaptRegisterHandler, &MSR_PKG_1_POWER_LIMIT_bits);
		adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_2");
		x86_adapt_get_setting(fd, adaptRegisterHandler, &MSR_PKG_2_POWER_LIMIT_bits);
		MSR_PKG_POWER_LIMIT_bits = MSR_PKG_1_POWER_LIMIT_bits | MSR_PKG_2_POWER_LIMIT_bits;
		
		adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_RAM_POWER_LIMIT");
		x86_adapt_get_setting(fd, adaptRegisterHandler, &MSR_DRAM_POWER_LIMIT_bits);

		if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
			std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
	#elif defined HAVE_GEOPM_H
		MSR_PKG_POWER_LIMIT_bits = geopm_signal_to_field(pio.read_signal(RAPL_PCKG_POWER_LIMIT, pio.signal_domain_type(RAPL_PCKG_POWER_LIMIT), 0));
		MSR_DRAM_POWER_LIMIT_bits = geopm_signal_to_field(pio.read_signal(RAPL_RAM_POWER_LIMIT, pio.signal_domain_type(RAPL_RAM_POWER_LIMIT), 0));
	#endif
		defaultSettings.defaultPowerCapPKG = MSR_PKG_POWER_LIMIT_bits;
		defaultSettings.defaultPowerCapRAM = MSR_DRAM_POWER_LIMIT_bits;
#elif defined _AMD_ && defined HAVE_E_SMI_E_SMI_H
		uint32_t pcap;
		if (!esmi_socket_power_cap_get(0, &pcap))
		{
			defaultSettings.defaultPowerCapPKG = pcap;
		}
		
		//supported power limit range is not checked by MERIC
/*		if (!esmi_socket_power_cap_max_get(0, &pcap))
		{
			defaultSettings.maxPowerCapPKG = pcap;
		}
		//no explicit minimum power limit
*/
#endif

// GPU POWERcap and FREQUENCIES ----------------------------------------------//
#ifdef HAVE_NVML_H
		if (NVML::getGPUsPerNode() > 0)
		{
			if (nvmlDeviceGetDefaultApplicationsClock(NVML::getGPUHandle(0), NVML_CLOCK_MEM, &defaultSettings.defaultGPUMemFreq) == NVML_SUCCESS)
				defaultSettings.defaultGPUMemFreq *= 1e6;
			else
				defaultSettings.defaultGPUMemFreq = 0;
			if (nvmlDeviceGetDefaultApplicationsClock(NVML::getGPUHandle(0), NVML_CLOCK_SM, &defaultSettings.defaultGPUSMFreq) == NVML_SUCCESS)
				defaultSettings.defaultGPUSMFreq *= 1e6;
			else
				defaultSettings.defaultGPUSMFreq = 0;
			if (nvmlDeviceGetPowerManagementLimit(NVML::getGPUHandle(0), &defaultSettings.defaultPowerCapGPU) == NVML_SUCCESS)
			{
				if (nvmlDeviceGetPowerManagementLimitConstraints(NVML::getGPUHandle(0), &defaultSettings.minPowerCapGPU, &defaultSettings.maxPowerCapGPU) != NVML_SUCCESS)
				{
					defaultSettings.maxPowerCapGPU = 0;
					defaultSettings.minPowerCapGPU = 0;
				}
			}			
			else
				defaultSettings.defaultPowerCapGPU = 0;
		}
		getAvailableGPUfrequencies(defaultSettings.availableGPUFreq);
#endif


#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
        MERIC_INFO << "FORCE INIT - readng GPU power and freuency limits\n";
		if (ROCM::getGPUsPerNode() > 0)
		{
            MERIC_INFO << "read GPU power and frequency settings";
			defaultSettings.defaultGPUSMFreq = 0;
			defaultSettings.defaultGPUMemFreq = 0;
			uint64_t maxPowerCapGPU, minPowerCapGPU, defaultPowerCapGPU;
			if (rsmi_dev_power_cap_range_get(0, 0, &maxPowerCapGPU, &minPowerCapGPU) != RSMI_STATUS_SUCCESS)
			{
				defaultSettings.minPowerCapGPU = 0;
				defaultSettings.maxPowerCapGPU = 0;
			}
			else
			{
				defaultSettings.minPowerCapGPU = static_cast<unsigned int>(minPowerCapGPU / 1e3);
				defaultSettings.maxPowerCapGPU = static_cast<unsigned int>(maxPowerCapGPU / 1e3);
			}
			
			if (rsmi_dev_power_cap_default_get(0, &defaultPowerCapGPU) != RSMI_STATUS_SUCCESS)
			{
				defaultSettings.defaultPowerCapGPU = 0;
			}
			else
			{
				defaultSettings.defaultPowerCapGPU = static_cast<unsigned int>(defaultPowerCapGPU / 1e3);
			}
		}
		getAvailableGPUfrequencies(defaultSettings.availableGPUFreq);
#endif


// store the configuration to the file ---------------------------------------//
		std::string defaultSettingFile = std::string(MERIC_TMP_PATH)+"/systemDefault.json";
		//test if the file has been modified since we tried to read it
		if (stat(defaultSettingFile.c_str(), &info ) != 0 || modifTime == info.st_mtim)
		{
			std::ofstream defaultRestoreFile;
			defaultRestoreFile.open (defaultSettingFile);
			
			if (restoreJson == "")
				restoreJson = "{\n";
			else
			{
				restoreJson = restoreJson.substr(0,restoreJson.find_last_of("}")-1) + ",\n";
			}
			
			//CPU
			std::string CPUname;
			getCPUbrandString(CPUname);

			restoreJson += "\t\""+CPUname+"\": {\n";
			
			restoreJson += "\t\t\"CF\" : {\n\t\t\t\"GOVERNOR\" : \"" + scalingGovernor + "\",\n"
				        +  "\t\t\t\"TURBO\" : " + std::to_string(defaultSettings.maxTurboFreq) + ",\n"
				        +  "\t\t\t\"MAX\" : " + std::to_string(defaultSettings.maxCoreFreq) + ",\n"
				        +  "\t\t\t\"MIN\" : " + std::to_string(defaultSettings.minCoreFreq) + ",\n"
#ifdef _INTEL_
				        +  "\t\t\t\"ENERGY_PERF_BIAS\" : " + std::to_string(defaultSettings.IA32_ENERGY_PERF_BIAS_default) + ",\n"
#elif defined _AMD_
				        +  "\t\t\t\"HWCR\" : " + std::to_string(defaultSettings.MSR_HWCR_default) + ",\n"
#endif
				        +  "\t\t\t\"NOMINAL\" : " + std::to_string(defaultSettings.nominalCoreFreq) + ",\n"
				        +  "\t\t\t\"DEFAULT\" : " + std::to_string(defaultSettings.defaultCoreFreq);
			if (!defaultSettings.availableCoreFreq.empty())
			{
				restoreJson += ",\n\t\t\t\"AVAILABLE\" : [";
				for (auto cFreq : defaultSettings.availableCoreFreq)
					restoreJson += " "+std::to_string(cFreq) + ",";
				restoreJson.pop_back();
				restoreJson += " ]";
			}
			restoreJson += "\n\t\t}";

#if defined _INTEL_ || defined __arm__
			restoreJson += ",\n\t\t\"UFS\" : {\n\t\t\t\"MAX\" : " + std::to_string(defaultSettings.maxUncoreFreq) + ",\n"
				        +  "\t\t\t\"MIN\" : " + std::to_string(defaultSettings.minUncoreFreq) + ",\n"
				        +  "\t\t\t\"DEFAULT\" : " + std::to_string(defaultSettings.defaultUncoreFreq) + "\n\t\t}";
#endif

#if defined _INTEL_ || ( defined _AMD_ && defined HAVE_E_SMI_E_SMI_H )
			restoreJson += ",\n\t\t\"PWRCAP\" : {\n\t\t\t\"PKG\" : "  +std::to_string(defaultSettings.defaultPowerCapPKG)
			#ifdef _INTEL_
				        +  ",\n\t\t\t\"DRAM\" : " +std::to_string(defaultSettings.defaultPowerCapRAM)
			#endif
				        +  "\n\t\t}";
#endif

			restoreJson += "\n\t}";

			//GPU
#if defined HAVE_NVML_H || defined HAVE_ROCM_SMI_ROCM_SMI_H
			std::string GPUname;
			getGPUbrandString(GPUname);
            MERIC_INFO << "GPU NAME: " << GPUname << std::endl;
			restoreJson += ",\n\t\""+GPUname+"\": {\n"
			            +  "\t\t\"FREQ\" : {\n\t\t\t\"DEFAULT_MEMORY\" : " + std::to_string(defaultSettings.defaultGPUMemFreq) + ",\n"
			            +  "\t\t\t\"DEFAULT_MULTIPROCESSOR\" : " + std::to_string(defaultSettings.defaultGPUSMFreq);
			if (defaultSettings.availableGPUFreq.size() == 0)
			{
				restoreJson += "\n\t\t},\n";
			}
			else
			{
				restoreJson += ",\n\t\t\t\"AVAILABLE\" : {\n";
				for (auto memFreq : defaultSettings.availableGPUFreq)
				{
					std::string GPUfreqString;
					std::stringstream ss;
					std::copy(memFreq.second.begin(), memFreq.second.end(), std::ostream_iterator<uint>(ss, ", "));
					GPUfreqString = ss.str();
					restoreJson += "\t\t\t\t\"" + std::to_string(memFreq.first) + "\" : [" + GPUfreqString.substr(0, GPUfreqString.size()-2) + " ],\n";
				}
				restoreJson = restoreJson.substr(0, restoreJson.size()-2) + "\n\t\t\t}\n"
					        + "\n\t\t},\n";
			}
			restoreJson += "\t\t\"PWRCAP\" : {\n\t\t\t\"DEFAULT\" : "  +std::to_string(defaultSettings.defaultPowerCapGPU) +",\n"
					        +  "\t\t\t\"MAX\" : "+std::to_string(defaultSettings.maxPowerCapGPU) + ",\n"
					        +  "\t\t\t\"MIN\" : "+std::to_string(defaultSettings.minPowerCapGPU) + "\n\t\t}"
					        +  "\n\t}\n";
#else
			restoreJson += "\n";
#endif
			restoreJson += "}\n";
			if (defaultRestoreFile.is_open())
			{
				defaultRestoreFile << restoreJson;
				defaultRestoreFile.close();
			}
		}
	}

	MERIC_INFO
		<< "CF"  << std::endl
		<< "\tDRV " << scalingDriver << std::endl
		<< "\tGOV " << scalingGovernor << std::endl
		<< "\tTRB " << defaultSettings.maxTurboFreq << std::endl
		<< "\tMAX " << defaultSettings.maxCoreFreq << std::endl
		<< "\tMIN " << defaultSettings.minCoreFreq << std::endl
		<< "\tNOM " << defaultSettings.nominalCoreFreq << std::endl
		<< "\tDEF " << defaultSettings.defaultCoreFreq << std::endl
		<< "\tAVA " << defaultSettings.availableCoreFreq.size() << std::endl
#if defined _INTEL_
		<< "\tBIA " << defaultSettings.IA32_ENERGY_PERF_BIAS_default << std::endl
#elif defined _AMD_
		<< "\tHWC " << defaultSettings.MSR_HWCR_default << std::endl
#endif
#if defined _INTEL_ || defined __arm__
		<< "UnCF" << std::endl
		<< "\tMAX " << defaultSettings.maxUncoreFreq << std::endl
		<< "\tMIN " << defaultSettings.minUncoreFreq << std::endl
		<< "\tDEF " << defaultSettings.defaultUncoreFreq << std::endl
#endif
#if defined _INTEL_ || defined _AMD_
		<< "CPU PWRcap" << std::endl
		<< "\tPKG " << defaultSettings.defaultPowerCapPKG << std::endl
	#if defined _INTEL_
		<< "\tRAM " << defaultSettings.defaultPowerCapRAM << std::endl
	#endif
#endif
#if defined HAVE_NVML_H || defined HAVE_ROCM_SMI_ROCM_SMI_H
		<< "GPU def freqs" << std::endl
		<< "\tMEM " << defaultSettings.defaultGPUMemFreq << std::endl
		<< "\tSMP " << defaultSettings.defaultGPUSMFreq << std::endl
		<< "GPU freqs list size" << std::endl
		<< "\tMEM " << defaultSettings.availableGPUFreq.size() << std::endl
		<< "\tSMP " << defaultSettings.availableGPUFreq.begin()->second.size() << std::endl
		<< "GPU PWRcap" << std::endl
		<< "\tDEF " << defaultSettings.defaultPowerCapGPU << std::endl
		<< "\tMAX " << defaultSettings.maxPowerCapGPU << std::endl
		<< "\tMIN " << defaultSettings.minPowerCapGPU << std::endl
#endif
		;
}

/**
 * inits the libraries used for tuning the environment settings and activates 
 * CPU core frequency tuning (and "uncore" for Jetson system) if possible
 * and sets the required system parameters
 * system - the target system (required for ARM Jetson)
 * defaultParameter - environment settings that should be set directly after system init
 * rank - MPI rank
 **/
void Environment::init(SYSTEM_TYPE system, RegionParameters defaultParameter, int rank)
{
#ifdef HAVE_X86_ADAPT_H
	if (x86_adapt_init())
	{
		std::cerr << "X86ADAPT ERROR: Unable to initialize\n";
//		exit(1);
	}
#endif

#ifdef HAVE_MSR_CORE_H
	if (init_msr())
	{
		std::cerr << "LIBMSR ERROR: Unable to initialize\n";
//		exit(1);
	}
#elif defined HAVE_GEOPM_H
	pio.save_control();
#endif

#ifdef HAVE_E_SMI_E_SMI_H
	if (esmi_init() != ESMI_SUCCESS)
	{
		std::cerr << "ESMI ERROR: Unable to initialize\n";
	}
#endif

#ifdef HAVE_NVML_H
	NVML::init();
#endif
	
#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	ROCM::init();
#endif
	
	initHWSizes();
	RAPL::initCountersUnits();
	systemType = system;

	if(!rank)
	{
		readSystemDefaultSettings();
//ACTIVATE DVFS
		if (defaultParameter.frequency != 0)
		{
			activatedDVFS = activateDVFS(); //TODO void
		}

//ACTIVATE UFS
		if (defaultSettings.defaultUncoreFreq != -1)
		{
			activatedUFS = isUFSPossible();
		}

//ACTIVATE GPU tuning
#if defined HAVE_NVML_H || defined HAVE_ROCM_SMI_ROCM_SMI_H
		if (defaultParameter.frequencyGPU != 0 || defaultParameter.powerCapGPU != 0)
		{
			getGPUTuningMethod();
		}
#endif
#ifdef __arm__
		if (systemType == SYS_JETSON)
		{
			if (defaultParameter.uncoreFrequency != 0 && access("/sys/kernel/debug/clock/override.emc/state", W_OK) == 0)
			{
				std::ofstream governorFile;
				governorFile.open ("/sys/kernel/debug/clock/override.emc/state");
				if (governorFile.is_open())
				{
					governorFile << "1";
					governorFile.close();
				}
				if (governorFile)
				{
					activatedUFS = isUFSPossible();
					MERIC_INFO << "MERIC: unCORE FREQUENCY scaling activated\n";
				}
			}
		}
#endif

//ACTIVATE POWERCAP
#if defined _INTEL_ || defined _AMD_
		if (defaultSettings.defaultPowerCapPKG != 0x0)
		{
			activatedPWRCAP = isPWRCAPpossible();
		}
#endif
		//set the settings that user wants to apply for the application
		frequency(defaultParameter.frequency);
		uncoreFrequency(defaultParameter.uncoreFrequency, defaultParameter.uncoreFrequency);
		CPUpowercap(defaultParameter.powerCapPKG, defaultParameter.powerCapPKGtime);
		GPUfrequency(defaultParameter.frequencyGPUmem, defaultParameter.frequencyGPU);
		GPUpowercap(defaultParameter.powerCapGPU);
	}
	threadNumber(defaultParameter.nthreads);

/*	//Set affinity
#ifdef HAVE_OMP_H
	#pragma omp parallel
	{
		#pragma omp critical
		MERIC_INFO << omp_get_thread_num() << " -- " << sched_getcpu() << std::endl;
		cpu_set_t cpu_set;
		CPU_ZERO(&cpu_set);
		CPU_SET(sched_getcpu(), &cpu_set); //current position
		//CPU_SET(omp_get_thread_num(), &cpu_set); //position according its thread num
		sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
	}
#endif
*/
	MERIC_INFO << "Environment::init - DONE\n";
}

/**
 * change number of active threads
 * if the input number of active threads is zero, return immediately
 **/
void Environment::threadNumber(int nthreads)
{
	timespec start_time, stop_time;
	clock_gettime(CLOCK_REALTIME, &start_time);

	if (nthreads <= 0)
		return;
#ifdef HAVE_OMP_H
	if (nthreads != omp_get_num_threads())
	{
		if (!omp_in_parallel())
		{
			MERIC_INFO << "OPENMP set num threads to: " << nthreads << std::endl;
			omp_set_num_threads(nthreads);
		}
		else
		{
			MERIC_INFO << "MERIC WARNING: parallel region, num threads did not change to: " << nthreads << std::endl;
		}
	}
#endif
/*
#ifdef HAVE_CILK_CILK_API_H
	//using cilk
	__cilkrts_end_cilk();  
	__cilkrts_set_param("nworkers", std::to_string(nthreads).c_str());
#endif
*/
	clock_gettime(CLOCK_REALTIME, &stop_time);
	MERIC_INFO << "MERIC number of OpenMP threads changed in " << mutils::timeToDouble(stop_time - start_time)*1000 << " ms\n";
}

/**
 * set CPU core frequency [Hz] to specific socket
 * if the input frequency is zero, return immediately
 * if no socket specified (implicit -1) set the same frequency to all sockets
 **/
void Environment::frequency(uint64_t freq, int socket)
{
	int socket_min, socket_max;
	timespec start_time, stop_time;
	clock_gettime(CLOCK_REALTIME, &start_time);

	if (freq == 0)
	{
		return;
	}

	if (activatedDVFS == NO_DVFS && usedDVFS == false) //hard reset sets usedDVFS to true
	{
		return;
	}

	// upper limit is checked only if the information about the turbo frequency is available
	if (freq < defaultSettings.minCoreFreq || (defaultSettings.maxTurboFreq > 1 && freq > defaultSettings.maxTurboFreq))
	{
		std::cerr << "CPU frequency has not changed to invalid value: " << freq << std::endl;
		return;
	}

	usedDVFS = true;

	MERIC_INFO << "MERIC set frequency to: " << freq;
	if (socket != -1 && socket < SOCKETS_PER_NODE)
	{
		MERIC_INFO << "; socket: " << socket;
		socket_min = socket;
		socket_max = socket;
	}
	else
	{
		socket_min = 0;
		socket_max = SOCKETS_PER_NODE-1;
	}

#if defined _INTEL_ && (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) //only when having MSR_SAFE the Environment::isHWPactive() method may return true
	if (activatedDVFS == HWP)
	{
		std::ofstream coreFile;
		for (int soc=socket_min; soc<=socket_max; soc++)
		{
			for (int cpu : mapCoresToSocket[soc])
			{
				for (int thrd : mapThreadsToCore[cpu])
				{
					coreFile.open ("/sys/devices/system/cpu/cpu"+std::to_string(thrd)+"/cpufreq/scaling_max_freq");
					if (coreFile.is_open())
					{
						coreFile << freq / 1000;
						coreFile.close();
					}
					if(!coreFile)
					{
						std::cerr << "MERIC ERROR: failed to write to /sys/devices/system/cpu/cpu"<<thrd<<"/cpufreq/scaling_max_freq\n";
					}
					coreFile.open ("/sys/devices/system/cpu/cpu"+std::to_string(thrd)+"/cpufreq/scaling_min_freq");
					if (coreFile.is_open())
					{
						coreFile << freq / 1000;
						coreFile.close();
					}
					if(!coreFile)
					{
						std::cerr << "MERIC ERROR: failed to write to /sys/devices/system/cpu/cpu"<<thrd<<"/cpufreq/scaling_min_freq\n";
					}
				}
			}
		}
		clock_gettime(CLOCK_REALTIME, &stop_time);
		MERIC_INFO << "MERIC CPU core frequency changed in " << mutils::timeToDouble(stop_time - start_time)*1000 << " ms\n";
		return;
	}
#endif
#if defined _INTEL_ && (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined IA32_PERF_CTL
	#ifdef HAVE_MSR_CORE_H
	MERIC_INFO << "; using LIBMSR\n";
	#else
	MERIC_INFO << "; using MSR_SAFE\n";
	#endif

	if (activatedDVFS == INT_PSTATE || activatedDVFS == HWP || activatedDVFS == INT_CPUFREQ)
	{
		for (int soc=socket_min; soc<=socket_max; soc++)
		{
			for (int cpu : mapCoresToSocket[soc])
			{
				for (int thrd : mapThreadsToCore[cpu])
				{
					write_msr(thrd, IA32_PERF_CTL, (freq/100000000) << IA32_PERF_CTL_shift);
				}
			}
		}
	}
	else //if (activatedDVFS == USRSPACE_FILE || activatedDVFS == USRSPACE_LIB)
	{
		std::ofstream coreFile;
		for (int soc=socket_min; soc<=socket_max; soc++)
		{
			for (int cpu : mapCoresToSocket[soc])
			{
				for (int thrd : mapThreadsToCore[cpu])
				{
					coreFile.open ("/sys/devices/system/cpu/cpu"+std::to_string(thrd)+"/cpufreq/scaling_setspeed");
					if (coreFile.is_open())
					{
						coreFile << freq / 1000;
						coreFile.close();
					}
					if(!coreFile)
					{
						std::cerr << "MERIC ERROR: failed to write to /sys/devices/system/cpu/cpu"<<thrd<<"/cpufreq/scaling_setspeed\n";
					}
				}
			}
		}
		if (freq > getNominalFrequency())
		{
			for (int soc=socket_min; soc<=socket_max; soc++)
			{
				for (int cpu : mapCoresToSocket[soc])
				{
					for (int thrd : mapThreadsToCore[cpu])
					{
						write_msr(thrd, IA32_PERF_CTL, (freq/100000000) << IA32_PERF_CTL_shift);
					}
				}
			}
		}
	}

#elif defined HAVE_X86_ADAPT_H
	//else if (activatedDVFS == INT_PSTATE || activatedDVFS == USRSPACE_LIB)
	MERIC_INFO << "; using X86ADAPT\n";
	int adaptHandler = x86_adapt_lookup_ci_name(X86_ADAPT_CPU, "Intel_Target_PState");
	if (adaptHandler < 0)
	{
		std::cerr << "X86ADAPT ERROR: frequency settings not allowed\n";
		return;
	}

	for (int soc=socket_min; soc<=socket_max; soc++)
	{
		for (int cpu : mapCoresToSocket[soc])
		{
			for (int thrd : mapThreadsToCore[cpu])
			{
				int fd = x86_adapt_get_device(X86_ADAPT_CPU, thrd);
				if (fd < 0)
				{
					std::cerr << "X86ADAPT ERROR: error while accessing file descriptor\n";
//					exit(2);
				}

				if(x86_adapt_set_setting(fd, adaptHandler, (freq/100000000) << IA32_PERF_CTL_shift) != 8)
				{
					std::cerr << "X86ADAPT ERROR: error while setting core frequecy\n";
//					exit(3);
				}

				if (x86_adapt_put_device(X86_ADAPT_CPU, thrd))
				{
					std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
				}
			}
		}
	}
#elif defined HAVE_GEOPM_H
	//else if (activatedDVFS == INT_PSTATE || activatedDVFS == USRSPACE_LIB)
	MERIC_INFO << "; using GEOPM\n";
	for (int soc=socket_min; soc<=socket_max; soc++)
	{
		for (int cpu : mapCoresToSocket[soc])
		{
			for (int thrd : mapThreadsToCore[cpu])
			{
				pio.write_control("FREQUENCY", pio.control_domain_type("FREQUENCY"), thrd, freq);
			}
		}
	}
#elif defined HAVE_CPUFREQ_H
	//else if (activatedDVFS == USRSPACE_LIB)
	MERIC_INFO << "; using CPUFREQ\n";
	for (int soc=socket_min; soc<=socket_max; soc++)
	{
		for (int cpu : mapCoresToSocket[soc])
		{
			for (int thrd : mapThreadsToCore[cpu])
			{
				if(cpufreq_set_frequency(thrd,freq) != 0)
				{
					std::cerr << "CPUFREQ ERROR: failed to set frequency for cpu "<< thrd << std::endl;
				}
			}
		}
	}

	MERIC_INFO << "CPUFREQ: core frequency is " << cpufreq_get_freq_kernel(mapThreadsToCore[mapCoresToSocket[socket_min].front()].front()) << std::endl;
#elif defined _AMD_ && defined HAVE_E_SMI_E_SMI_H
/**
 * AMD CPU cores are grouped into Core Complexes (CCXes), all cores grouped into 
 * a common CCX share a Last-Level (L3) cache. Each CCX includes a clock 
 * generator that supports a resolution of 25 MHz. If all cores of a CCX are not 
 * programmed for the same boost limit frequency, then the lower-frequency cores 
 * are limited to a frequency resolution that can be as low as 20% of the 
 * requested frequency. If the specified boost limit frequency of a core is not 
 * supported, then the processor selects the next lower supported frequency.
 **/
	//else if (activatedDVFS == OTHER)
	MERIC_INFO << "; using ESMI\n";
	for (int soc=socket_min; soc<=socket_max; soc++)
	{
		for (int cpu : mapCoresToSocket[soc])
		{
			//test TODO for (int thrd : mapThreadsToCore[cpu])
			esmi_core_boostlimit_set(cpu, freq/1e6);
		}
	}
#elif defined _AMD_ && defined HAVE_MSR_SAFE && defined MSR_PSTATE_CTL && defined MSR_HWCR
	//else if (activatedDVFS == OTHER)
	MERIC_INFO << "; using MSR_SAFE\n";
	if (defaultSettings.availableCoreFreq.empty())
	{
		getAvailableCF(defaultSettings.availableCoreFreq);
		if (defaultSettings.availableCoreFreq.empty())
		{
			std::cerr << "MERIC ERROR: cannot set CPU core frequency, unknown available P-States\n";
			return;
		}
	}

	if (freq > defaultSettings.availableCoreFreq[0]) //Turbo ON, Pstate 0
	{
		for (int soc=socket_min; soc<=socket_max; soc++)
		{
			for (int cpu : mapCoresToSocket[soc])
			{
				//test TODO for (int thrd : mapThreadsToCore[cpu])
				write_msr(cpu, MSR_HWCR, defaultSettings.MSR_HWCR_default & (0xFFFFFFFFFFFFFFFF ^ MSR_HWCR_boost_disable_mask));
				write_msr(cpu, MSR_PSTATE_CTL, 0x0); //P0
			}
		}
	}
	else //Turbo OFF, Pstate X
	{
		int PstateIdx = 0;
		while (PstateIdx < defaultSettings.availableCoreFreq.size()-1 && freq <= defaultSettings.availableCoreFreq[PstateIdx+1])
		{
			PstateIdx++;
		}
		for (int soc=socket_min; soc<=socket_max; soc++)
		{
			for (int cpu : mapCoresToSocket[soc])
			{
				//test TODO for (int thrd : mapThreadsToCore[cpu])
				write_msr(cpu, MSR_HWCR, defaultSettings.MSR_HWCR_default | MSR_HWCR_boost_disable_mask);
				write_msr(cpu, MSR_PSTATE_CTL, PstateIdx); //P-idx
			}
		}
	}
#else
	//using sysfs
	MERIC_INFO << "; using SYSFS\n";
	std::string scalingFile = "";
	if (activatedDVFS == USRSPACE_FILE)
	{
		scalingFile = "scaling_setspeed";
	}
	else if (activatedDVFS == INT_CPUFREQ)
	{
		scalingFile = "scaling_max_freq";
	}
	
	std::ofstream coreFile;
	for (int soc=socket_min; soc<=socket_max; soc++)
	{
		for (int cpu : mapCoresToSocket[soc])
		{
			for (int thrd : mapThreadsToCore[cpu])
			{
				coreFile.open ("/sys/devices/system/cpu/cpu"+std::to_string(thrd)+"/cpufreq/"+scalingFile);
				if (coreFile.is_open())
				{
					coreFile << freq / 1000;
					coreFile.close();
				}
				if(!coreFile)
				{
					std::cerr << "MERIC ERROR: failed to write to /sys/devices/system/cpu/cpu"<<thrd<<"/cpufreq/"+scalingFile+"\n";
				}
			}
		}
	}
#endif
	clock_gettime(CLOCK_REALTIME, &stop_time);
	MERIC_INFO << "MERIC CPU core frequency changed in " << mutils::timeToDouble(stop_time - start_time)*1000 << " ms\n";
}

/**
 * set CPU uncore frequency limits [Hz] to specific socket 
 * if the input frequency is zero, return immediately
 * if no socket specified (implicit -1) set the same frequency to all sockets
 **/
void Environment::uncoreFrequency(uint64_t maxFreq, uint64_t minFreq, int socket)
{
#if !defined _INTEL_ && !defined __arm__
	return;
#else
	int socket_min, socket_max;
	timespec start_time, stop_time;
	clock_gettime(CLOCK_REALTIME, &start_time);

	if (maxFreq == 0 || minFreq == 0)
		return;
	
	if (activatedUFS == UFS_WAY_NONE)
	{
		return; //UFS not supported
	}
		
	if (maxFreq > defaultSettings.maxUncoreFreq || minFreq < defaultSettings.minUncoreFreq || maxFreq < minFreq)
	{
		std::cerr << "Uncore frequency has not changed to invalid value: " << maxFreq << std::endl;
		MERIC_INFO << defaultSettings.maxUncoreFreq << " - " << defaultSettings.minUncoreFreq << std::endl;
		return;
	}
	usedUFS = true;

	MERIC_INFO << "MERIC set uncore frequency to: "<< maxFreq;
	if (maxFreq != minFreq)
	{
		MERIC_INFO << " - " << minFreq;
	}

	if (socket != -1 && socket < SOCKETS_PER_NODE)
	{
		MERIC_INFO << "; socket: " << socket;
		socket_min = socket;
		socket_max = socket;
	}
	else
	{
		socket_min = 0;
		socket_max = SOCKETS_PER_NODE-1;
	}

	if(activatedUFS == UFS_WAY_TPMI_SYSFS)
	{
		MERIC_INFO << "; using TPMI SYSFS\n";

		std::ofstream file;

		for(int die=socket_min; die <= socket_max; ++die)
		{
			file.open("/sys/devices/system/cpu/intel_uncore_frequency/package_0" + std::to_string(die) + "_die_00/min_freq_khz");
			if(file.is_open())
			{
				file << minFreq / 1000;
				file.close();
			}

			file.open("/sys/devices/system/cpu/intel_uncore_frequency/package_0" + std::to_string(die) + "_die_00/max_freq_khz");
			if(file.is_open())
			{
				file << maxFreq / 1000;
				file.close();
			}
		}

		clock_gettime(CLOCK_REALTIME, &stop_time);
		MERIC_INFO << "MERIC uncore frequency changed in " << mutils::timeToDouble(stop_time - start_time)*1000 << " ms\n";
		return;
	}

	#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_UNCORE_RATIO_LIMIT
	#ifdef HAVE_MSR_CORE_H
	MERIC_INFO << "; using LIBMSR\n";
	#else
	MERIC_INFO << "; using MSR_SAFE\n";
	#endif
	uint64_t MSR_UNCORE_RATIO_LIMIT_bits = ((minFreq/100000000) << MSR_UNCORE_RATIO_LIMIT_min_shift) + maxFreq/100000000;
	for (int die=socket_min; die<=socket_max; die++)
	{
		write_msr(mapCoresToSocket[die][0], MSR_UNCORE_RATIO_LIMIT, MSR_UNCORE_RATIO_LIMIT_bits);
	}
	#elif defined HAVE_X86_ADAPT_H
	MERIC_INFO << "; using X86ADAPT\n";
	if (x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO") < 0)
	{
		std::cerr << "X86ADAPT ERROR: uncore frequency settings not allowed\n";
		return;
	}
	
	int adaptHandlerUncoreMax = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO");
	int adaptHandlerUncoreMin = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MIN_RATIO");
	for (int die=socket_min; die<=socket_max; die++)
	{
		int fd = x86_adapt_get_device(X86_ADAPT_DIE, die);
		if (fd < 0)
		{
			std::cerr << "X86ADAPT ERROR: error while accessing file descriptor\n";
//			exit(2);
		}

		auto setFreq = [&] (int adaptHandler, uint64_t freq)
		{
			if(x86_adapt_set_setting(fd, adaptHandler, freq) != 8)
			{
				std::cerr << "X86ADAPT ERROR: error while setting uncore frequency\n";
//				exit(3);
			}
		};

		setFreq(adaptHandlerUncoreMax, maxFreq/100000000);
		setFreq(adaptHandlerUncoreMin, minFreq/100000000);
	
		if (x86_adapt_put_device(X86_ADAPT_DIE, die))
			std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
	}
	#elif defined HAVE_GEOPM_H
	MERIC_INFO << "; using GEOPM\n";
	for (int die=socket_min; die<=socket_max; die++)
	{
		pio.write_control("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", pio.control_domain_type("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO"), die, minFreq);
		pio.write_control("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", pio.control_domain_type("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO"), die, maxFreq);
	}
	#else
	// SYS_JETSON
	std::ofstream uncoreFile;
	uncoreFile.open ("/sys/kernel/debug/clock/override.emc/rate");
	if (uncoreFile.is_open())
	{
		uncoreFile << maxFreq;
		uncoreFile.close();
	}
	if (uncoreFile)
		MERIC_INFO << "MERIC set uncore frequency to: "<< maxFreq << std::endl;
	else
		std::cerr << "MERIC ERROR: failed to write to /sys/kernel/debug/clock/override.emc/rate\n";
	#endif
	clock_gettime(CLOCK_REALTIME, &stop_time);
	MERIC_INFO << "MERIC uncore frequency changed in " << mutils::timeToDouble(stop_time - start_time)*1000 << " ms\n";
#endif
}

/**
 * set CPU power limit
 *	- Intel, set RAPL long-term Package power domain limit
 *	- AMD, set HSMP power limit, does not support time window
 * WARNING: range of supported power limits is not checked, any power limit is set
 * power [mW], if zero return immediately
 * timeWindow [ms], if zero the time window will not change
 * if no socket specified (implicit -1) set the same limit to all sockets
 **/
void Environment::CPUpowercap(uint64_t power, uint64_t timeWindow, int socket)
{
	if (power == 0)
		return;

#ifdef HAVE_MSR_CORE_H
	MERIC_INFO << "LIBMSR set CPU powercap to: "<< power << " mW";
#elif defined _INTEL_ && defined HAVE_MSR_SAFE
	MERIC_INFO << "MSR set CPU powercap to: "<< power << " mW";
#elif defined HAVE_X86_ADAPT_H
	MERIC_INFO << "X86ADAPT set CPU powercap to: "<< power << " mW";
#elif defined HAVE_GEOPM_H
	MERIC_INFO << "GEOPM set CPU powercap to: "<< power << " mW";
#elif defined HAVE_E_SMI_E_SMI_H
	MERIC_INFO << "ESMI set CPU powercap to: "<< power << " mW";
#else
	return;
#endif
	if (timeWindow != 0)
	{
		MERIC_INFO << " for " << timeWindow << " ms";
	}
	usedPWRCAP = true;

	int socket_min, socket_max;
	if (socket != -1 && socket < SOCKETS_PER_NODE)
	{
		MERIC_INFO << "; socket: " << socket << std::endl;
		socket_min = socket;
		socket_max = socket;
	}
	else
	{
		MERIC_INFO << std::endl;
		socket_min = 0;
		socket_max = SOCKETS_PER_NODE-1;
	}	

#ifdef _INTEL_
	uint64_t MSR_PKG_POWER_LIMIT_bits;
	#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_PKG_POWER_LIMIT
	read_msr(0, MSR_PKG_POWER_LIMIT, &MSR_PKG_POWER_LIMIT_bits);
	#elif defined HAVE_X86_ADAPT_H
	int adaptRegisterHandler, fd;
	fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
	if (fd < 0)
	{
		std::cerr << "X86ADAPT RAPL INIT error while accessing file descriptor\n";
		return;
	}

	adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_1");
	x86_adapt_get_setting(fd, adaptRegisterHandler, &MSR_PKG_POWER_LIMIT_bits);

	if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
		std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
	#elif defined HAVE_GEOPM_H
	MSR_PKG_POWER_LIMIT_bits = geopm_signal_to_field(pio.read_signal(RAPL_PCKG_POWER_LIMIT, pio.signal_domain_type(RAPL_PCKG_POWER_LIMIT), 0));
	#endif
	
	MSR_PKG_POWER_LIMIT_bits = (MSR_PKG_POWER_LIMIT_bits & (~MSR_PKG_POWER_LIMIT_mask)) | (((uint64_t)(power / (1000*RAPL::getUnit(RAPL::PCKG_POWER_LIMIT)))) & MSR_PKG_POWER_LIMIT_mask) | MSR_PKG_POWER_LIMIT_enable_mask;


	#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_X86_ADAPT_H
	if (timeWindow != 0)
	{
		/** PKG Time window size equation
		 * Time limit = 2^Y * (1.0 + Z/4.0) * Time_Unit
		 * Y = 0 - 31
		 * Z = 0 - 3 => 1.0 - 1.75
		 * DRAM uses different equation
		 **/
		uint64_t Y = 0;
		uint64_t Z = 0;
		timeWindow = timeWindow/(1000 * RAPL::getUnit(RAPL::POWER_LIMIT_TIMEWINDOW));
		if (timeWindow > 1)
		{
			Y = log2(timeWindow);
			int baseY = pow(2,Y);
			int baseZ = baseY/4;
			int ZLimitPart = timeWindow - baseY;
			Z = ZLimitPart / baseZ;
			if (ZLimitPart % baseZ > (baseZ/2))
				Z++;
		}
		MSR_PKG_POWER_LIMIT_bits = (MSR_PKG_POWER_LIMIT_bits & (~(MSR_PKG_POWER_LIMIT_time_z_mask | MSR_PKG_POWER_LIMIT_time_y_mask))) | 
			(Y << MSR_PKG_POWER_LIMIT_time_y_shift) | (Z << MSR_PKG_POWER_LIMIT_time_z_shift);
	}

	for (int die=socket_min; die<=socket_max; die++)
	{
		#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_PKG_POWER_LIMIT
		write_msr(mapCoresToSocket[die][0], MSR_PKG_POWER_LIMIT, MSR_PKG_POWER_LIMIT_bits);
		#elif defined HAVE_X86_ADAPT_H
		fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, die);
		if (fd < 0)
		{
			std::cerr << "X86ADAPT RAPL INIT error while accessing file descriptor\n";
			return;
		}
		
		adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_1");
		x86_adapt_set_setting(fd, adaptRegisterHandler, MSR_PKG_POWER_LIMIT_bits);

		if (x86_adapt_put_device(X86_ADAPT_DIE, die))
			std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
		#endif
	}
	#elif defined HAVE_GEOPM_H
	for (int die=socket_min; die<=socket_max; die++)
	{
		pio.write_control(RAPL_PCKG_POWER_LIMIT_1, pio.control_domain_type(RAPL_PCKG_POWER_LIMIT_1), die, power/1000.0);
	}
	if (timeWindow != 0)
	{
		for (int die=socket_min; die<=socket_max; die++)
		{
			pio.write_control(RAPL_POWER_LIMIT_TIMEWINDOW, pio.control_domain_type(RAPL_POWER_LIMIT_TIMEWINDOW), die, timeWindow/1000.0);
		}
	}
	#endif
#elif defined _AMD_ && defined HAVE_E_SMI_E_SMI_H
	for (int die=socket_min; die<=socket_max; die++)
	{
		if (esmi_socket_power_cap_set(die, power) != ESMI_SUCCESS)
		{
			std::cerr << "ESMI ERROR: failed to set power limit\n";
		}
	}
#endif

}

/**
 * set GPU streaming multiprocessors and memory frequencies
 * freqMEM [Hz], frequency of the memory, ignore if zero
 * freqSM [Hz], frequency of the streaming multiprocessors, ignore if zero
 * if no specific GPU specified (implicit -1) set the frequencies to all the available GPUs
 **/
void Environment::GPUfrequency(uint freqMEM, uint freqSM, int gpu)
{
#if defined HAVE_NVML_H
	if (activatedGPUtuning == NVML_API)
	{
		MERIC_INFO << "NVML set GPU frequency to: "<< freqMEM/1000 << "," << freqSM/1000 << " kHz\n";
	}
	else if (activatedGPUtuning == NVIDIA_SMI)
	{
		MERIC_INFO << "NVIDIA-SMI set GPU frequency to: "<< freqMEM/1000 << "," << freqSM/1000 << " kHz\n";
	}
	else
	{
		return;
	}
#elif defined HAVE_ROCM_SMI_ROCM_SMI_H
	if (activatedGPUtuning == ROCM_SMI_API)
	{
		MERIC_INFO << "ROCM SMI API set GPU frequency to: "<< freqMEM/1000 << "," << freqSM/1000 << " kHz\n";
	}
	else if (activatedGPUtuning == ROCM_SMI_UTIL)
	{
		MERIC_INFO << "ROCM SMI UTIL set GPU frequency to: "<< freqMEM/1000 << "," << freqSM/1000 << " kHz\n";
	}
	else
	{
		return;
	}
#else
	return;
#endif

#if defined HAVE_NVML_H || defined HAVE_ROCM_SMI_ROCM_SMI_H
	if (freqMEM == 0 && freqSM == 0)
	{
		return;
	}

	//find out the nearest available memory frequency and nearest available frequency of the streaming multiprocessors
	std::map<uint, std::vector<uint>>::iterator MEMit;
	#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	int SMIndex, memIndex;
	#endif
	if (freqMEM == 0)
	{
		if (currentGPUMemFreq == 0)
		{
			currentGPUMemFreq = defaultSettings.defaultGPUMemFreq;
		}
		freqMEM = currentGPUMemFreq;
		MEMit = defaultSettings.availableGPUFreq.find(freqMEM);
	}
	else
	{
		if (freqMEM < defaultSettings.availableGPUFreq.begin()->first || freqMEM > defaultSettings.availableGPUFreq.rbegin()->first)
		{
			std::cerr << "MERIC ERROR: GPU memory frequency has not changed to invalid value: " << freqMEM << " Hz\n";
			std::cerr << defaultSettings.availableGPUFreq.begin()->first << " - " << defaultSettings.availableGPUFreq.rbegin()->first << std::endl;
			return;
		}
		MEMit = defaultSettings.availableGPUFreq.find(freqMEM);
		if (MEMit == defaultSettings.availableGPUFreq.end())
		{
			MEMit = defaultSettings.availableGPUFreq.upper_bound(freqMEM);
		}
		#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
			memIndex = std::distance( MEMit, defaultSettings.availableGPUFreq.end()) - 1;
		#endif
		freqMEM = MEMit->first;
	}
	
	if (freqSM == 0)
	{
		if (currentGPUSMFreq == 0)
		{
			currentGPUSMFreq = defaultSettings.defaultGPUSMFreq;
		}
		freqSM = currentGPUSMFreq;
	}
	//no else branch - if the MEM frequency has changed, the SM frequency may not be available
	if (freqSM < MEMit->second.front() || freqSM > MEMit->second.back())
	{
		std::cerr << "MERIC ERROR: GPU streaming multiprocessor frequency has not changed to invalid value: " << freqSM << " Hz\n";
		std::cerr << MEMit->second.front() << " - " << MEMit->second.back() << std::endl;
		return;
	}
	std::vector<uint>::iterator SMit = std::find(MEMit->second.begin(), MEMit->second.end(), freqSM);
	if(SMit == MEMit->second.end())
	{
		SMit = std::upper_bound(MEMit->second.begin(), MEMit->second.end(), freqSM);
	}
	#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	SMIndex = std::distance(SMit, MEMit->second.end()) - 1;
	MERIC_INFO << "SM index: " << SMIndex << " MEM index: " << memIndex << std::endl;
	#endif
	freqSM = *SMit;
	
	int min, max;
	if (gpu == -1)
	{
		min = 0;
		#if defined HAVE_NVML_H
		max = NVML::getGPUsPerNode();
		#elif defined HAVE_ROCM_SMI_ROCM_SMI_H
		max = ROCM::getGPUsPerNode();
		#endif
		MERIC_INFO << "GPUs to change frequency: " << max << std::endl;
	}
	else
	{
		if (NVML::getGPUsPerNode() <= gpu || ROCM::getGPUsPerNode() <= gpu)
		{
			std::cerr << "MERIC ERROR: specifying frequency of GPU id " << gpu << " from 0-" << NVML::getGPUsPerNode() << std::endl;
			return;
		}
		min = gpu;
		max = gpu+1;
	}
	
	#ifdef HAVE_NVML_H
	MERIC_INFO << "NVML set GPU frequency MEM: " << freqMEM << "\tSM: " << freqSM << std::endl;
	#endif

	#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	MERIC_INFO << "RSMI set GPU frequency MEM: " << freqMEM << "\tSM: " << freqSM << std::endl;
	#endif

	usedGPUDVFS = true;
	
	#ifdef HAVE_NVML_H
	if (activatedGPUtuning == NVML_API)
	{
		for (int id=min; id<max; id++)
		{
			if (nvmlDeviceSetApplicationsClocks(NVML::getGPUHandle(id), freqMEM/1e6, freqSM/1e6) != NVML_SUCCESS)
			{
				std::cerr << "MERIC ERROR: NVML failed to set GPU frequency\n";
			}
		}
	}
	else if (activatedGPUtuning == NVIDIA_SMI)
	{
		const std::string command = "sudo nvidia-smi -ac "+std::to_string(freqMEM/1000000)+","+std::to_string(freqSM/1000000);
		for (int id=min; id<max; id++)
		{
			system((command+" -i "+std::to_string(id)+" 2> /dev/null > /dev/null \n").c_str());
		}
	}
	#endif

	#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	if (activatedGPUtuning == ROCM_SMI_API)
	{
		for (int id=min; id<max; id++)
		{
			if(rsmi_dev_gpu_clk_freq_set(id, RSMI_CLK_TYPE_SYS, 0b1 << SMIndex) != RSMI_STATUS_SUCCESS)
			{
				std::cerr << "MERIC ERROR: RSMI failed to set GPU SM frequency\n";
			}
			if(rsmi_dev_gpu_clk_freq_set(id, RSMI_CLK_TYPE_MEM, 0b1 << memIndex) != RSMI_STATUS_SUCCESS)
			{
				std::cerr << "MERIC ERROR: RSMI failed to set GPU Memory frequency\n";
			}
		}
	}
	else if (activatedGPUtuning == ROCM_SMI_UTIL)
	{
		
		const std::string sm_command = "sudo rocm-smi --setsclk " + std::to_string(SMIndex);
		const std::string mem_command = "sudo rocm-smi --setmclk " + std::to_string(memIndex);
		for (int id=min; id<max; id++)
		{
			system((sm_command+" -d "+std::to_string(id)+" 2> /dev/null > /dev/null \n").c_str());
			system((mem_command+" -d "+std::to_string(id)+" 2> /dev/null > /dev/null \n").c_str());
		}
	}
	#endif
	currentGPUMemFreq = freqMEM;
	currentGPUSMFreq  = freqSM;
#endif

}

/**
 * change GPU power capping limit
 * power [mW], if zero return immediately
 * GPU, if no explicit GPU id specified (implicit -1), set the same limit to all GPUs
 **/
void Environment::GPUpowercap(uint power, int gpu)
{
	if (power == 0)
	{
		return;
	}
#if defined HAVE_NVML_H
	if (activatedGPUtuning == NVML_API)
	{
		MERIC_INFO << "NVML set GPU powercap to: "<< power << " mW\n";
	}
	else if (activatedGPUtuning == NVIDIA_SMI)
	{
		MERIC_INFO << "NVIDIA-SMI set GPU powercap to: "<< power << " mW\n";
	}
	else
	{
		return;
	}
#elif defined HAVE_ROCM_SMI_ROCM_SMI_H
	if (activatedGPUtuning == ROCM_SMI_API)
	{
		MERIC_INFO << "ROCM SMI API set GPU powercap to: "<< power << " mW\n";
	}
	else if (activatedGPUtuning == ROCM_SMI_UTIL)
	{
		MERIC_INFO << "ROCM SMI UTIL set GPU powercap to: "<< power << " mW\n";
	}
	else
	{
		return;
	}
#else
	return;
#endif

	if (power < defaultSettings.minPowerCapGPU || power > defaultSettings.maxPowerCapGPU)
	{
		std::cerr << "MERIC ERROR: GPU power limit has not changed to invalid value: " << power << std::endl;
		MERIC_INFO << defaultSettings.maxPowerCapGPU << " - " << defaultSettings.minPowerCapGPU << std::endl;
		return;
	}

	int min = 0;
	int max;

	if (gpu == -1)
	{
		min = 0;
		#if defined HAVE_NVML_H
		max = NVML::getGPUsPerNode();
		#elif defined HAVE_ROCM_SMI_ROCM_SMI_H
		max = ROCM::getGPUsPerNode();
		#endif
	}
	else
	{
		if (NVML::getGPUsPerNode() <= gpu)
		{
			std::cerr << "MERIC ERROR: NVML specifying power limit (" << power/1000.0 << "W) of GPU id " << gpu << " from 0-" << NVML::getGPUsPerNode() << std::endl;
			return;
		}
		
		min = gpu;
		max = gpu+1;
	}
		
#ifdef HAVE_NVML_H	
	usedGPUPWRCAP = true;
	if (activatedGPUtuning == NVML_API)
	{
		for (int id=min; id<max; id++)
		{
			if (nvmlDeviceSetPowerManagementLimit(NVML::getGPUHandle(id), power) != NVML_SUCCESS)
			{
				std::cerr << "MERIC ERROR: NVML setting power limit failed\n";
			}
		}
	}
	else if (activatedGPUtuning == NVIDIA_SMI)
	{
		const std::string command = "sudo nvidia-smi -pl "+std::to_string(power/1e3);
		for (int id=min; id<max; id++)
		{
			system((command+" -i "+std::to_string(id)+" 2> /dev/null > /dev/null \n").c_str());
		}
	}
#endif


#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	usedGPUPWRCAP = true;
	if (activatedGPUtuning == ROCM_SMI_API)
	{
		for (int id=min; id<max; id++)
		{
			if (rsmi_dev_power_cap_set(id, 0, power * 1e3) != RSMI_STATUS_SUCCESS)
			{
				std::cerr << "MERIC ERROR: RSMI setting power limit failed\n";
			}
		}
	}
	else if (activatedGPUtuning == ROCM_SMI_UTIL)
	{
		const std::string command = "sudo rocm-smi --setpoweroverdrive "+std::to_string(static_cast<uint>(power / 1e3));
        MERIC_INFO << command << std::endl;
        system(command.c_str());
		/*for (int id=min; id<max; id++)
		{
			system((command+" -d "+std::to_string(id)+" 2> /dev/null > /dev/null \n").c_str());
		}*/
	}
#endif

}



/**
 * sets the default scaling governor, CPU core frequency, CPU uncore frequency limits and powercap limits
 * the restore happens only if the settings have changed during the MERIC run
 * to force the restore in all cases set input hardReset to true (implicit false)
 **/
void Environment::restoreSystemSettings(bool hardReset)
{
#ifdef HAVE_GEOPM_H
	if (!hardReset && (activatedDVFS || activatedUFS || usedPWRCAP))
		pio.restore_control();
#endif
	if (hardReset)
	{
		//DVFS_WAY activatedDVFS is not specified, because it requires the way how it was activated, which we may not know
		usedDVFS = true;
		activatedUFS = isUFSPossible();
		usedUFS = true;
		activatedPWRCAP = true;
		usedPWRCAP = true;
		usedGPUDVFS = true;
		usedGPUPWRCAP = true;
	}

	if (activatedDVFS != NO_DVFS || hardReset)
	{
		if (usedDVFS)
		{
			if (defaultSettings.defaultCoreFreq > 0)
				frequency(defaultSettings.defaultCoreFreq);
			else if (defaultSettings.maxTurboFreq > 1)
				frequency(defaultSettings.maxTurboFreq);
			else
				frequency(defaultSettings.maxCoreFreq);
		}
		if ((activatedDVFS == USRSPACE_FILE || activatedDVFS == USRSPACE_LIB || hardReset) && scalingGovernor != "userspace")
		{
			std::ofstream governorFile;
			for (int soc=0; soc<SOCKETS_PER_NODE; soc++)
			{
				for (int cpu : mapCoresToSocket[soc])
				{
					for (int thread : mapThreadsToCore[cpu])
					{
						governorFile.open ("/sys/devices/system/cpu/cpu"+std::to_string(thread)+"/cpufreq/scaling_governor");
						if (governorFile.is_open())
						{
							governorFile << scalingGovernor;
							governorFile.close();
						}
					}
				}
			}
		}
		if (activatedDVFS == INT_PSTATE || hardReset)
		{
			std::ofstream defSet;
			if (defaultSettings.maxTurboFreq > 0)
			{
				defSet.open ("/sys/devices/system/cpu/intel_pstate/no_turbo");
				if (defSet.is_open())
				{
					defSet << "0";
					defSet.close();
				}
			}
			
			for (int soc=0; soc<SOCKETS_PER_NODE; soc++)
			{
				for (int cpu : mapCoresToSocket[soc])
				{
					for (int thread : mapThreadsToCore[cpu])
					{
						defSet.open ("/sys/devices/system/cpu/cpu"+std::to_string(thread)+"/cpufreq/scaling_max_freq");
						if (defSet.is_open())
						{
							defSet << defaultSettings.maxCoreFreq / 1000;
							defSet.close();
						}
#if defined _INTEL_ && (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined IA32_ENERGY_PERF_BIAS
						write_msr(thread, IA32_ENERGY_PERF_BIAS, defaultSettings.IA32_ENERGY_PERF_BIAS_default);
#endif
					}
				}
			}
		}
	}

	if (activatedUFS != UFS_WAY_NONE)
	{
		if(usedUFS)
		{
			if (defaultSettings.defaultUncoreFreq > 0)
				uncoreFrequency(defaultSettings.defaultUncoreFreq, defaultSettings.defaultUncoreFreq);
			else
				uncoreFrequency(defaultSettings.maxUncoreFreq, defaultSettings.minUncoreFreq);
		}

		if (systemType == SYS_JETSON)
		{
			//uncore
			std::ofstream memTuningFile;
			memTuningFile.open ("/sys/kernel/debug/clock/override.emc/state");
			if (memTuningFile.is_open())
			{
				memTuningFile << "0";
				memTuningFile.close();
			}
		}
	}

	if (usedPWRCAP)
	{
#if defined _INTEL_
		if (defaultSettings.defaultPowerCapPKG != 0x0)
	#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_PKG_POWER_LIMIT
		{
			for (int die=0; die<SOCKETS_PER_NODE; die++)
				write_msr(mapCoresToSocket[die][0], MSR_PKG_POWER_LIMIT, defaultSettings.defaultPowerCapPKG);
		}
	#elif defined HAVE_X86_ADAPT_H
		{
			for (int die=0; die<SOCKETS_PER_NODE; die++)
			{
				const int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, die);
				if (fd < 0)
				{
					std::cerr << "X86ADAPT RAPL INIT error while accessing file descriptor\n";
					return;
				}
				int adaptRegisterHandler;
				uint64_t POWER_LIMIT_bits = defaultSettings.defaultPowerCapPKG & MSR_PKG_POWER_LIMIT_mask;
				adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_1");
				x86_adapt_set_setting(fd, adaptRegisterHandler, POWER_LIMIT_bits);
				
				POWER_LIMIT_bits = defaultSettings.defaultPowerCapPKG >> MSR_PKG_POWER_LIMIT_power2_shift;
				adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_2");
				x86_adapt_set_setting(fd, adaptRegisterHandler, POWER_LIMIT_bits);

				if (x86_adapt_put_device(X86_ADAPT_DIE, die))
					std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
			}
		}
	#elif defined HAVE_GEOPM_H
		{
			for (int die=0; die<SOCKETS_PER_NODE; die++)
			{
				double val = (defaultSettings.defaultPowerCapPKG & MSR_PKG_POWER_LIMIT_clamping_mask) >> MSR_PKG_POWER_LIMIT_clamping_shift;
				pio.write_control("MSR::PKG_POWER_LIMIT:PL1_CLAMP_ENABLE", pio.control_domain_type("MSR::PKG_POWER_LIMIT:PL1_CLAMP_ENABLE"), die, val);
				val = (defaultSettings.defaultPowerCapPKG & MSR_PKG_POWER_LIMIT_enable_mask) >> MSR_PKG_POWER_LIMIT_enable_shift;
				pio.write_control("MSR::PKG_POWER_LIMIT:PL1_LIMIT_ENABLE", pio.control_domain_type("MSR::PKG_POWER_LIMIT:PL1_LIMIT_ENABLE"), die, val);
				val = ((defaultSettings.defaultPowerCapPKG & MSR_PKG_POWER_LIMIT_mask) >> MSR_PKG_POWER_LIMIT_shift) * RAPL::getUnit(RAPL_PCKG_POWER_LIMIT);
				pio.write_control("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT", pio.control_domain_type("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT"), die, val);

				uint64_t time_y = (defaultSettings.defaultPowerCapPKG & MSR_PKG_POWER_LIMIT_time_y_mask) >> MSR_PKG_POWER_LIMIT_time_y_shift;
				uint64_t time_z = (defaultSettings.defaultPowerCapPKG & MSR_PKG_POWER_LIMIT_time_z_mask) >> MSR_PKG_POWER_LIMIT_time_z_shift;
				val = pow(2.0, time_y) * (1.0 + time_z/4.0) * RAPL::getUnit(RAPL_POWER_LIMIT_TIMEWINDOW);
				pio.write_control("MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW", pio.control_domain_type("MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW"), die, val);
			}
		}
	#else
		;
	#endif
//		if (defaultSettings.defaultPowerCapRAM != 0x0) //currently not tuned in MERIC
//		...
#elif defined _AMD_ && defined HAVE_E_SMI_E_SMI_H
		CPUpowercap(defaultSettings.defaultPowerCapPKG);
#endif
	}

#ifdef HAVE_NVML_H
	//GPU frequencies
	if (usedGPUDVFS)
	{
		if (activatedGPUtuning == NVML_API)
		{
			for (int id=0; id<NVML::getGPUsPerNode(); id++)
			{
				if (nvmlDeviceResetApplicationsClocks(NVML::getGPUHandle(id)) != NVML_SUCCESS)
				{
					std::cerr << "MERIC ERROR: NVML failed to reset GPU frequency\n";
				}
			}
		}
		else if (activatedGPUtuning == NVIDIA_SMI)
		{
			system("sudo nvidia-smi -rac 2> /dev/null > /dev/null \n");
		}
	}
	
	//GPU power capping limit
	if (usedGPUPWRCAP)
	{
		GPUpowercap(defaultSettings.defaultPowerCapGPU);
	}
#endif
	
#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	//GPU frequencies
	if (usedGPUDVFS)
	{
		if (activatedGPUtuning == ROCM_SMI_API)
		{

			for (int id=0; id<NVML::getGPUsPerNode(); id++)
			{
				if (rsmi_dev_perf_level_set(id, RSMI_DEV_PERF_LEVEL_AUTO) != RSMI_STATUS_SUCCESS)
				{
					std::cerr << "MERIC ERROR: RSMI failed to reset GPU frequency\n";
				}
			}
		}
		else if (activatedGPUtuning == ROCM_SMI_UTIL)
		{
			system("sudo rocm-smi --setperflevel auto 2> /dev/null > /dev/null \n");
		}
	}
	
	//GPU power capping limit
	if (usedGPUPWRCAP)
	{
		GPUpowercap(defaultSettings.defaultPowerCapGPU);
	}
#endif 
}

/**
 * call close functions of the libraries that were used for environment tuning
 **/
void Environment::close()
{
// NOTE: "nvmlShutdown()" is now called as part of NVML::close()
/*
#ifdef HAVE_NVML_H
	nvmlShutdown();
#endif
*/
#ifdef HAVE_X86_ADAPT_H
	x86_adapt_finalize();
#endif
#ifdef HAVE_MSR_CORE_H
	finalize_msr();
#endif
#ifdef HAVE_E_SMI_E_SMI_H
	esmi_exit();
#endif
}


