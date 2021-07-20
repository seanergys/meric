#include "environmentwrapper.h"
#include "raplwrapper.h"
#include "nvmlwrapper.h"

using namespace meric;

systemLimits Environment::defaultSettings;

int Environment::SOCKETS_PER_NODE = 1;
int Environment::CORES_PER_SOCKET = 1;
int Environment::THREADS_PER_CORE = 1;
std::map<int, std::vector<int>> Environment::mapThreadsToCore;
std::map<int, std::vector<int>> Environment::mapCoresToSocket;
SYSTEM_TYPE Environment::systemType = SYS_OTHER;

std::string Environment::scalingGovernor = "";
std::string Environment::scalingDriver = "";

bool Environment::usedDVFS = false;
bool Environment::usedUFS = false;
bool Environment::usedPWRCAP = false;
bool Environment::usedGPUDVFS = false;
bool Environment::usedGPUPWRCAP = false;
DVFS_WAY Environment::activatedDVFS = NO_DVFS;
bool Environment::activatedUFS = false;
bool Environment::activatedPWRCAP = false;
#ifdef HAVE_NVML_H
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
			if (line.substr(0,10) == "model name")
			{
				CPUname = line.substr(line.find(':')+2);
				break;
			}
		}
		CPUinfoFile.close();
	}
	if (CPUname != "")
		return;

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
#endif


#ifdef HAVE_NVML_H
/**
 * Get name of an Nvidia GPU
 * The name is an alphanumeric string that denotes a particular product.
 * It will not exceed 50 characters in length (including the null terminator).
 **/
void Environment::getGPUbrandString(std::string & GPUname)
{
	const int GPU_NAME_LENGTH = 50;
	char name [GPU_NAME_LENGTH];
	if (NVML::getGPUsPerNode() && nvmlDeviceGetName (NVML::getGPUHandle(0), name, GPU_NAME_LENGTH) == NVML_SUCCESS)
		GPUname = std::string(name);
	else
		GPUname = "";
}
#endif

/**
 * returns nominal CPU core frequency in Hz
 * if libmsr available uses MSR_PLATFORM_INFO register
 * else if it is x86 architecture uses CPUID instruction
 * else if acpi-cpufreq scaling driver is used returns value from the system file
 * else returns 0
 **/
#ifdef HAVE_MSR_SAFE
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

	frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_nominal_freq");
	if (frequencyBounderyFile.is_open())
	{
		frequencyBounderyFile >> freqBound;
		frequencyBounderyFile.close();
		frequency = std::stol(freqBound)*1000;
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
				frequency = (frequency-1000) * 1000;
			else
				frequency *= 1000;
		}
	}
	#ifdef __x86_64
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
	#endif
	return frequency;
}
#endif

/**
 * reads from the system list of available specific CPU core frequencies if specified
 * otherwise the list produced using 100 MHz step
 **/
void Environment::getAvailableCF(std::vector<long> & values)
{	//TODO test /sys/devices/system/cpu/cpu0/cpufreq/scaling_boost_frequencies
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
	else
	{
		for (long freq = defaultSettings.minCoreFreq; freq < defaultSettings.maxTurboFreq; freq+=100000000)
			values.push_back(freq);
	}
}

/**
 * creates list of available uncore frequencies
 * TODO !!! ARM system, that has memory frequency represented as the uncore frequency
 **/
void Environment::getAvailableUCF(std::vector<long> & values)
{
	for (long freq = defaultSettings.minUncoreFreq; freq < defaultSettings.maxUncoreFreq; freq+=100000000)
		values.push_back(freq);
}

#ifdef HAVE_NVML_H
/**
 * reads available GPU memory and corresponding streaming multiprocessor frequencies
 **/
void Environment::getAvailableGPUfrequencies(std::map<uint, std::vector<uint>> & values)
{
	if (NVML::getGPUsPerNode() == 0)
		return;
	
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
						std::cerr << "NVML ERROR: cannot read available GPU frequencies\n";
						return;
					}
					free(gpuFreq);
					gpuFreqCount = 0;
				}
				else
				{
					std::cerr << "NVML ERROR: cannot read available GPU frequencies\n";
					return;
				}
			}
		}
		else
		{
			std::cerr << "NVML ERROR: cannot read available GPU frequencies\n";
			return;
		}
		free(memFreq);
	}
	else
	{
		std::cerr << "NVML ERROR: cannot read available GPU frequencies\n";
	}
	return;
}
#endif

/**
 * returns maximum available non-AVX turbo CPU core frequency in Hz
 * when the information is not available, returns 0
 * note: AVX, AVX2 and AVX512 frequency limits register not available/documented
 **/
long Environment::getMaximumTurboFrequency()
{
	long frequency = -1;
#ifdef HAVE_MSR_SAFE
	uint64_t MSR_TURBO_RATIO_LIMIT_bits = 0x0;
	read_msr(0, MSR_TURBO_RATIO_LIMIT, &MSR_TURBO_RATIO_LIMIT_bits);
	MSR_TURBO_RATIO_LIMIT_bits = MSR_TURBO_RATIO_LIMIT_bits & MSR_TURBO_RATIO_LIMIT_mask;
	frequency = (MSR_TURBO_RATIO_LIMIT_bits & MSR_TURBO_RATIO_LIMIT_mask) * 100000000;
#else
	if (getScalingDriver() == "intel-pstate")
	{
		std::ifstream frequencyBounderyFile("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
		if (frequencyBounderyFile.is_open())
		{
			std::string freqBound;
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
			frequency = std::stol(freqBound) * 1000;
		}
	}
#endif
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
 * enabled / disabled / not available / ""
 **/
#ifdef __x86_64
bool Environment::isHWPactive(std::string &status)
{
	status = "";
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
		#ifdef HAVE_MSR_SAFE
			uint64_t IA32_PM_ENABLE_bits;
			read_msr(0, IA32_PM_ENABLE, &IA32_PM_ENABLE_bits);
			HWP = IA32_PM_ENABLE_bits & IA32_PM_ENABLE_mask;
			status = HWP ? "enabled" : "disabled";
		#endif
	}
	else
		status = "not available";

	return HWP;
}
#endif

/**
 * tests if the CPU core frequency scaling is possible
 * returns possitive number if yes, otherwise zero
 **/
DVFS_WAY Environment::isDVFSpossible()
{
	#ifdef __x86_64
	std::string status;
	if (isHWPactive(status))
		return NO_DVFS;
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
#if defined HAVE_MSR_SAFE || defined HAVE_X86_ADAPT_H || defined HAVE_CPUFREQ_H || HAVE_GEOPM_H
		return USRSPACE_LIB;
#else
		if (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", W_OK) == 0)
			return USRSPACE_FILE;
		else
			return NO_DVFS;
#endif
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
#if defined HAVE_MSR_SAFE || defined HAVE_X86_ADAPT_H || defined HAVE_CPUFREQ_H || HAVE_GEOPM_H
				return USRSPACE_LIB;
#else
				if (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", W_OK) == 0)
					return USRSPACE_FILE;
				else
					return NO_DVFS;
#endif
			}
		}
		frequencyBounderyFile.close();
	}

#if defined HAVE_MSR_SAFE
	if (getScalingDriver() == "intel_pstate" && 
	   access("/sys/devices/system/cpu/intel_pstate/no_turbo", W_OK) == 0 &&
	   access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", W_OK) == 0)
		return INT_PSTATE; //requires also IA32_ENERGY_PERF_BIAS register in the whitelist
#endif
std::cout << "NO DVFS" << std::endl;
	return NO_DVFS;
}

/**
 * activates DVFS on the system
 * if the way of DVFS using (DVFS_WAY enum) is know, can be passed as argument
 * returns true in success, otherwise false
 **/
DVFS_WAY Environment::activateDVFS(DVFS_WAY way)
{
	if (way == NO_DVFS)
		way = isDVFSpossible();

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
						governorFileO << "userspace";
						governorFileO.close();
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
		defSet << "1";
		defSet.close();
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
					defSet << defaultSettings.minCoreFreq / 1000; //kHz
					defSet.close();
					if (!defSet)
					{
						std::cerr << "MERIC ERROR: Failed to write to /sys/devices/system/cpu/cpu"<<thread<<"/cpufreq/scaling_max_freq\n";
						return NO_DVFS;
					}
				}			
			}
		}
#ifdef HAVE_MSR_SAFE
		for (int soc=0; soc<SOCKETS_PER_NODE; soc++)
		{
			for (int cpu : mapCoresToSocket[soc])
				for (int thread : mapThreadsToCore[cpu])
					write_msr(thread, IA32_ENERGY_PERF_BIAS, 0);
		}
#endif
		//p_state may still rewrite the IA32_PERF_CTL register during a short time period, so we wait
		usleep(800000); //700 ms is not enough
	}
	
	return way;
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
 * detects the system topology (which thread belongs to which core; which cores belong to which socket) and sets following variables - system sizes:
 *  - SOCKETS_PER_NODE
 *  - CORES_PER_SOCKET
 *  - THREADS_PER_CORE
 **/
void Environment::initHWSizes()
{
	std::string topoStr;
	std::ifstream topoFile;
	
	topoFile.open("/sys/devices/system/cpu/online");
	topoFile >> topoStr;
	topoFile.close();
	std::vector<int> onlineThreads;
	stringOfIDsToVector(topoStr, onlineThreads);
	
	while(onlineThreads.size())
	{
		topoFile.open("/sys/devices/system/cpu/cpu"+std::to_string(onlineThreads[0])+"/topology/thread_siblings_list");
		topoFile >> topoStr;
		topoFile.close();
		std::vector<int> threadSiblings;
		stringOfIDsToVector(topoStr, threadSiblings);
		mapThreadsToCore[threadSiblings.back()] = threadSiblings;
		
		if (threadSiblings.size() > THREADS_PER_CORE)
			THREADS_PER_CORE = threadSiblings.size();
		
		for(int thrd : threadSiblings)
			onlineThreads.erase(std::remove(onlineThreads.begin(), onlineThreads.end(), thrd), onlineThreads.end());
	}
	
	for (const auto &core : mapThreadsToCore)
	{
		topoFile.open("/sys/devices/system/cpu/cpu"+std::to_string(core.first)+"/topology/physical_package_id");
		topoFile >> topoStr;
		topoFile.close();
		
		const int soc = std::stoi(topoStr);
		
		if (mapCoresToSocket.count(soc))
		{
			mapCoresToSocket[soc].push_back(core.first);
			if (CORES_PER_SOCKET < mapCoresToSocket[soc].size())
				CORES_PER_SOCKET++;
		}
		else
			mapCoresToSocket[soc] = {core.first};
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
	std::string defaultSettingFile = std::string(MERIC_PATH)+"/build/systemDefault.json";
	
	struct stat info;
	if(stat(defaultSettingFile.c_str(), &info) == 0)
	{
		modifTime = info.st_mtim;
		std::ifstream jsonFile (defaultSettingFile);
		std::string line = "";
		if (jsonFile.is_open())
		{
			while (getline(jsonFile,line))
				restoreJson += " " + line+"\n";
			jsonFile.close();
		}
		if (restoreJson == "")
			return false;
		
		const char * json = restoreJson.c_str();
		struct json_value_s* root = json_parse(json, strlen(json));
		if (root == NULL)
		{
			std::cerr << "MERIC ERROR: invalid " << defaultSettingFile << std::endl;
			rename(defaultSettingFile.c_str(), (defaultSettingFile+".err").c_str());
			restoreJson = "";
			return false;
		}

		std::string CPUname;
		getCPUbrandString(CPUname);
#ifdef HAVE_NVML_H
		std::string GPUname;
		getGPUbrandString(GPUname);
#endif
		struct json_object_element_s* data = ((struct json_object_s*)root->payload)->start;
		while(data != NULL)
		{
			if (std::string(data->name->string) == CPUname)
			{
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
							else if (std::string(dvfs->name->string) == "DEFAULT")
							{
								defaultSettings.defaultCoreFreq = std::stol(std::string(((struct json_string_s*)dvfs->value->payload)->string));
							}
							else if (std::string(dvfs->name->string) == "ENERGY_PERF_BIAS")
							{
								defaultSettings.IA32_ENERGY_PERF_BIAS_default = std::strtoull(std::string(((struct json_string_s*)dvfs->value->payload)->string).c_str(), NULL, 0);
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
					else if (std::string(architectureData->name->string) == "PWRCAP")
					{
						struct json_object_element_s* pwr = ((struct json_object_s*)architectureData->value->payload)->start;
						do
						{
							if (std::string(pwr->name->string) == "PKG")
							{
								defaultSettings.defaultPowerCapPKG = std::strtoull(std::string(((struct json_string_s*)pwr->value->payload)->string).c_str(), NULL, 0);
							}
							else if (std::string(pwr->name->string) == "DRAM")
							{
								defaultSettings.defaultPowerCapRAM = std::strtoull(std::string(((struct json_string_s*)pwr->value->payload)->string).c_str(), NULL, 0);
							}
							pwr = pwr->next;
						}while(pwr != NULL);
					}
					architectureData = architectureData->next;
				}
			}
#ifdef HAVE_NVML_H
			else if (std::string(data->name->string) == GPUname)
			{
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
			data = data->next;
		}
		free(root);
		return true;
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
		defaultSettings.maxTurboFreq = getMaximumTurboFrequency();
		std::string freqBound; //files have the values in KHz
		frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
		}
		defaultSettings.maxCoreFreq = std::stol(freqBound) * 1000;
		
		if (scalingGovernor == "userspace")
		{
			frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
			if (frequencyBounderyFile.is_open())
			{
				frequencyBounderyFile >> freqBound;
				frequencyBounderyFile.close();
			}
			defaultSettings.defaultCoreFreq = std::stol(freqBound) * 1000;
		}
		
		frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
		if (frequencyBounderyFile.is_open())
		{
			frequencyBounderyFile >> freqBound;
			frequencyBounderyFile.close();
		}
		defaultSettings.minCoreFreq = std::stol(freqBound) * 1000;
#ifdef HAVE_MSR_SAFE
		read_msr(0, IA32_ENERGY_PERF_BIAS, &defaultSettings.IA32_ENERGY_PERF_BIAS_default);
#endif

// CPU UnCORE freq -----------------------------------------------------------//
		defaultSettings.defaultUncoreFreq = 0;
#ifdef HAVE_MSR_SAFE
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
#else	//Jetson memory frequencies
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
		uint64_t MSR_PKG_POWER_LIMIT_bits = 0x0;
		uint64_t MSR_DRAM_POWER_LIMIT_bits = 0x0;
#ifdef HAVE_MSR_SAFE
		read_msr(0, MSR_PKG_POWER_LIMIT, &MSR_PKG_POWER_LIMIT_bits);
		read_msr(0, MSR_DRAM_POWER_LIMIT, &MSR_DRAM_POWER_LIMIT_bits);
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

// GPU POWERcap and FREQUENCIES ----------------------------------------------//
		defaultSettings.defaultGPUMemFreq = 0;
		defaultSettings.defaultGPUSMFreq = 0;
		defaultSettings.defaultPowerCapGPU = 0;
		defaultSettings.minPowerCapGPU = 0;
		defaultSettings.maxPowerCapGPU = 0;
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

// store the configuration to the file ---------------------------------------//
		std::string defaultSettingFile = std::string(MERIC_PATH)+"/build/systemDefault.json";
		struct stat info; //test if the file has been modified since we tried to read it
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
				        +  "\t\t\t\"DEFAULT\" : " + std::to_string(defaultSettings.defaultCoreFreq) + ",\n"
				        +  "\t\t\t\"ENERGY_PERF_BIAS\" : " + std::to_string(defaultSettings.IA32_ENERGY_PERF_BIAS_default) + "\n\t\t},\n";

			restoreJson += "\t\t\"UFS\" : {\n\t\t\t\"MAX\" : " + std::to_string(defaultSettings.maxUncoreFreq) + ",\n"
				        +  "\t\t\t\"MIN\" : " + std::to_string(defaultSettings.minUncoreFreq) + ",\n"
				        +  "\t\t\t\"DEFAULT\" : " + std::to_string(defaultSettings.defaultUncoreFreq) + "\n\t\t},\n";

			restoreJson += "\t\t\"PWRCAP\" : {\n\t\t\t\"PKG\" : "  +std::to_string(defaultSettings.defaultPowerCapPKG) + ",\n"
				        +  "\t\t\t\"DRAM\" : " +std::to_string(defaultSettings.defaultPowerCapRAM) +"\n\t\t}";

			restoreJson += "\n\t}";

			//GPU
#ifndef HAVE_NVML_H
			restoreJson += "\n";
#else
			std::string GPUname;
			getGPUbrandString(GPUname);
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
#endif
			restoreJson += "}\n";
			defaultRestoreFile << restoreJson;
			defaultRestoreFile.close();
		}
	}

	MERIC_INFO
		<< "CF"  << std::endl
		<< "\tDRV " << scalingDriver << std::endl
		<< "\tGOV " << scalingGovernor << std::endl
		<< "\tTRB " << defaultSettings.maxTurboFreq << std::endl
		<< "\tMAX " << defaultSettings.maxCoreFreq << std::endl
		<< "\tMIN " << defaultSettings.minCoreFreq << std::endl
		<< "\tDEF " << defaultSettings.defaultCoreFreq << std::endl
		<< "\tBIA " << defaultSettings.IA32_ENERGY_PERF_BIAS_default << std::endl
		<< "UnCF" << std::endl
		<< "\tMAX " << defaultSettings.maxUncoreFreq << std::endl
		<< "\tMIN " << defaultSettings.minUncoreFreq << std::endl
		<< "\tDEF " << defaultSettings.defaultUncoreFreq << std::endl
		<< "CPU PWRcap" << std::endl
		<< "\tPKG " << defaultSettings.defaultPowerCapPKG << std::endl
		<< "\tRAM " << defaultSettings.defaultPowerCapRAM << std::endl
		<< "GPU def freqs" << std::endl
		<< "\tMEM " << defaultSettings.defaultGPUMemFreq << std::endl
		<< "\tSMP " << defaultSettings.defaultGPUSMFreq << std::endl
		<< "GPU freqs list size" << std::endl
		<< "\tMEM " << defaultSettings.availableGPUFreq.size() << std::endl
		<< "\tSMP " << defaultSettings.availableGPUFreq.begin()->second.size() << std::endl
		<< "GPU PWRcap" << std::endl
		<< "\tDEF " << defaultSettings.defaultPowerCapGPU << std::endl
		<< "\tMAX " << defaultSettings.maxPowerCapGPU << std::endl
		<< "\tMIN " << defaultSettings.minPowerCapGPU << std::endl;
	
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
		std::cerr << "X86ADAPT ERROR: init error\n";
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

#ifdef HAVE_NVML_H
	nvmlInit();
#endif
	NVML::init();
	
	initHWSizes();
	RAPL::initCountersUnits();
	systemType = system;

	if(!rank)
	{
		readSystemDefaultSettings();

//ACTIVATE DVFS
		if (defaultParameter.frequency != 0)
		{
			activatedDVFS = activateDVFS();
		}

//ACTIVATE UFS
#if defined HAVE_MSR_SAFE || defined HAVE_X86_ADAPT_H
		if (defaultSettings.defaultUncoreFreq != -1)
			activatedUFS = true;
#endif
		if (systemType == SYS_JETSON)
		{
			if (defaultParameter.uncoreFrequency != 0 && access("/sys/kernel/debug/clock/override.emc/state", W_OK) == 0)
			{
				std::ofstream governorFile;
				governorFile.open ("/sys/kernel/debug/clock/override.emc/state");
				governorFile << "1";
				governorFile.close();
				if (governorFile)
				{
					activatedUFS = true;
					MERIC_INFO << "MERIC: unCORE FREQUENCY scaling activated\n";
				}
			}
		}

//ACTIVATE POWERCAP
		if (defaultSettings.defaultPowerCapPKG != 0x0)
			activatedPWRCAP = true;


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
		return;

	if (!activatedDVFS)
		return;

	// upper limit is checked only if the information about the turbo frequency is available
	if (freq < defaultSettings.minCoreFreq || (defaultSettings.maxTurboFreq != -1 && freq > defaultSettings.maxTurboFreq))
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
	
#ifdef HAVE_MSR_SAFE
	MERIC_INFO << "; using LIBMSR\n";	
	for (int soc=socket_min; soc<=socket_max; soc++)
	{
		for (int cpu : mapCoresToSocket[soc])
		{
			write_msr(cpu, IA32_PERF_CTL, (freq/100000000) << IA32_PERF_CTL_shift);
		}
	}
#elif defined HAVE_X86_ADAPT_H
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
			int fd = x86_adapt_get_device(X86_ADAPT_CPU, cpu);
			if (fd < 0)
			{
				std::cerr << "X86ADAPT ERROR: error while accessing file descriptor\n";
//				exit(2);
			}

			if(x86_adapt_set_setting(fd, adaptHandler, (freq/100000000) << IA32_PERF_CTL_shift) != 8)
			{
				std::cerr << "X86ADAPT ERROR: error while setting core frequecy\n";
//				exit(3);
			}

			if (x86_adapt_put_device(X86_ADAPT_CPU, cpu))
				std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
		}
	}
#elif defined HAVE_CPUFREQ_H
	MERIC_INFO << "; using CPUFREQ\n";
	for (int soc=socket_min; soc<=socket_max; soc++)
	{
		for (int cpu : mapCoresToSocket[soc])
		{
			if(cpufreq_set_frequency(cpu,freq) != 0)
			{
				std::cerr << "CPUFREQ ERROR: failed to set frequency for cpu "<< cpu << std::endl;
			}
		}
	}
	MERIC_INFO << "CPUFREQ: core frequency is " << cpufreq_get_freq_kernel(nodeSize-1) << std::endl;
#elif defined HAVE_GEOPM_H
	MERIC_INFO << "; using GEOPM\n";
	for (int soc=socket_min; soc<=socket_max; soc++)
	{
		for (int cpu : mapCoresToSocket[soc])
		{
			pio.write_control("FREQUENCY", pio.control_domain_type("FREQUENCY"), cpu, freq);
		}
	}
#else
	std::ofstream coreFile;
	for (int soc=socket_min; soc<=socket_max; soc++)
	{
		for (int cpu : mapCoresToSocket[soc])
		{
			coreFile.open ("/sys/devices/system/cpu/cpu"+std::to_string(cpu)+"/cpufreq/scaling_setspeed");
			coreFile << freq / 1000;
			coreFile.close();
			if(!coreFile)
				std::cerr << "MERIC ERROR: failed to write to /sys/devices/system/cpu/cpu"<<cpu<<"/cpufreq/scaling_setspeed\n";
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
	int socket_min, socket_max;
	timespec start_time, stop_time;
	clock_gettime(CLOCK_REALTIME, &start_time);

	if (maxFreq == 0 || minFreq == 0)
		return;
	
	if (!activatedUFS)
		return; //UFS not supported
		
	if (maxFreq > defaultSettings.maxUncoreFreq || minFreq < defaultSettings.minUncoreFreq || maxFreq < minFreq)
	{
		std::cerr << "Uncore frequency has not changed to invalid value: " << maxFreq << std::endl;
		MERIC_INFO << defaultSettings.maxUncoreFreq << " - " << defaultSettings.minUncoreFreq << std::endl;
		return;
	}
	usedUFS = true;
#if defined HAVE_MSR_SAFE || defined HAVE_X86_ADAPT_H || defined HAVE_GEOPM_H
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
#endif

#ifdef HAVE_MSR_SAFE
	MERIC_INFO << "; using LIBMSR\n";
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
	uncoreFile << maxFreq;
	uncoreFile.close();
	if (uncoreFile)
		MERIC_INFO << "MERIC set uncore frequency to: "<< maxFreq << std::endl;
	else
		std::cerr << "MERIC ERROR: failed to write to /sys/kernel/debug/clock/override.emc/rate\n";
#endif
	clock_gettime(CLOCK_REALTIME, &stop_time);
	MERIC_INFO << "MERIC uncore frequency changed in " << mutils::timeToDouble(stop_time - start_time)*1000 << " ms\n";
}

/**
 * change RAPL power capping limit (PKG only) of a specific socket
 * power [mW], if zero return immediately
 * timeWindow [ms], if zero the time window will not change
 * if no socket specified (implicit -1) set the same limit to all sockets
 **/
void Environment::CPUpowercap(uint64_t power, uint64_t timeWindow, int socket)
{
	if (power == 0)
		return;

	int socket_min, socket_max;
#ifdef HAVE_MSR_CORE_H
	MERIC_INFO << "LIBMSR set CPU powercap to: "<< power << " mW";
#elif defined HAVE_MSR_SAFE
	MERIC_INFO << "MSR set CPU powercap to: "<< power << " mW";
#elif defined HAVE_X86_ADAPT_H
	MERIC_INFO << "X86ADAPT set CPU powercap to: "<< power << " mW";
#elif defined HAVE_GEOPM_H
	MERIC_INFO << "GEOPM set CPU powercap to: "<< power << " mW";
#else
	return;
#endif
	if (timeWindow != 0)
	{
		MERIC_INFO << " for " << timeWindow << " ms";
	}
	usedPWRCAP = true;

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

	uint64_t MSR_PKG_POWER_LIMIT_bits;
#ifdef HAVE_MSR_SAFE
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
	
	MSR_PKG_POWER_LIMIT_bits = (MSR_PKG_POWER_LIMIT_bits & (~MSR_PKG_POWER_LIMIT_mask)) | (((uint64_t)(power / (1000*RAPL::getUnit(RAPL_PCKG_POWER_LIMIT)))) & MSR_PKG_POWER_LIMIT_mask) | MSR_PKG_POWER_LIMIT_enable_mask;


#if defined HAVE_MSR_SAFE || defined HAVE_X86_ADAPT_H
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
		timeWindow = timeWindow/(1000 * RAPL::getUnit(RAPL_POWER_LIMIT_TIMEWINDOW));
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
	#ifdef HAVE_MSR_SAFE
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

}

/**
 * set GPU streaming multiprocessors and memory frequencies
 * freqMEM [Hz], frequency of the memory, ignore if zero
 * freqSM [Hz], frequency of the streaming multiprocessors, ignore if zero
 * if no specific GPU specified (implicit -1) set the frequencies to all the available GPUs
 **/
void Environment::GPUfrequency(uint freqMEM, uint freqSM, int gpu)
{

#ifndef HAVE_NVML_H
	return;
#else

	if (freqMEM == 0 && freqSM == 0)
		return;

	//find out the nearest available memory frequency and nearest available frequency of the streaming multiprocessors
	std::map<uint, std::vector<uint>>::iterator MEMit;
	if (freqMEM == 0)
	{
		if (currentGPUMemFreq == 0)
			currentGPUMemFreq = defaultSettings.defaultGPUMemFreq;
		freqMEM = currentGPUMemFreq;
		MEMit = defaultSettings.availableGPUFreq.find(freqMEM);
	}
	else
	{
		if (freqMEM < defaultSettings.availableGPUFreq.begin()->first || freqMEM > defaultSettings.availableGPUFreq.rbegin()->first)
		{
			std::cerr << "GPU memory frequency has not changed to invalid value: " << freqMEM << " Hz\n";
			return;
		}
		MEMit = defaultSettings.availableGPUFreq.find(freqMEM);
		if (MEMit == defaultSettings.availableGPUFreq.end())
			MEMit = defaultSettings.availableGPUFreq.upper_bound(freqMEM);
		freqMEM = MEMit->first;
	}
	
	if (freqSM == 0)
	{
		if (currentGPUSMFreq == 0)
			currentGPUSMFreq = defaultSettings.defaultGPUSMFreq;
		freqSM = currentGPUSMFreq;
	}
	//no else branch - if the MEM frequency has changed, the SM frequency may not be available
	if (freqSM < MEMit->second.front() || freqSM > MEMit->second.back())
	{
		std::cerr << "GPU streaming multiprocessor frequency has not changed to invalid value: " << freqSM << " Hz\n";
		return;
	}
	std::vector<uint>::iterator SMit = std::find(MEMit->second.begin(), MEMit->second.end(), freqSM);
	if(SMit == MEMit->second.end())
	{
		SMit = std::upper_bound(MEMit->second.begin(), MEMit->second.end(), freqSM);
	}
	freqSM = *SMit;
	
	int min, max;
	if (gpu == -1)
	{
		min = 0;
		max = NVML::getGPUsPerNode();
	}
	else
	{
		if (NVML::getGPUsPerNode() <= gpu)
		{
			std::cerr << "MERIC ERROR: specifying frequency of GPU id " << gpu << " from 0-" << NVML::getGPUsPerNode() << std::endl;
			return;
		}
		
		min = gpu;
		max = gpu+1;
	}

	MERIC_INFO << "NVML set GPU frequency MEM: " << freqMEM << "\tSM: " << freqSM << std::endl;
	usedGPUDVFS = true;
	
	for (int id=min; id<max; id++)
	{
		if (nvmlDeviceSetApplicationsClocks(NVML::getGPUHandle(id), freqMEM/1e6, freqSM/1e6) != NVML_SUCCESS)
			std::cerr << "NVML ERROR: failed to set GPU frequency\n";
	}
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
		return;
	
#ifdef HAVE_NVML_H
	MERIC_INFO << "NVML set GPU powercap to: "<< power << " mW\n";
#else
	return;
#endif

	if (power < defaultSettings.minPowerCapGPU || power > defaultSettings.maxPowerCapGPU)
	{
		std::cerr << "GPU power limit has not changed to invalid value: " << power << std::endl;
		MERIC_INFO << defaultSettings.maxPowerCapGPU << " - " << defaultSettings.minPowerCapGPU << std::endl;
		return;
	}

	int min = 0;
	int max;

	if (gpu == -1)
	{
		min = 0;
		max = NVML::getGPUsPerNode();
	}
	else
	{
		if (NVML::getGPUsPerNode() <= gpu)
		{
			std::cerr << "MERIC ERROR: specifying power limit (" << power/1000.0 << "W) of GPU id " << gpu << " from 0-" << NVML::getGPUsPerNode() << std::endl;
			return;
		}
		
		min = gpu;
		max = gpu+1;
	}
		
#ifdef HAVE_NVML_H	
	usedGPUPWRCAP = true;
	for (int id=min; id<max; id++)
	{
		if (nvmlDeviceSetPowerManagementLimit(NVML::getGPUHandle(id), power) != NVML_SUCCESS)
		{
			std::cerr << "NVML ERROR setting power limit failed\n";
		}
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

	if (activatedDVFS || hardReset)
	{
		if (usedDVFS || hardReset)
		{
			if (defaultSettings.defaultCoreFreq > 0)
				frequency(defaultSettings.defaultCoreFreq);
			else
				frequency(defaultSettings.maxTurboFreq);
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
						governorFile << scalingGovernor;
						governorFile.close();
					}
				}
			}
		}
		if (activatedDVFS == INT_PSTATE || hardReset)
		{
			std::ofstream defSet;
			defSet.open ("/sys/devices/system/cpu/intel_pstate/no_turbo");
			defSet << "0";
			defSet.close();
			
			for (int soc=0; soc<SOCKETS_PER_NODE; soc++)
			{
				for (int cpu : mapCoresToSocket[soc])
				{
					for (int thread : mapThreadsToCore[cpu])
					{
						defSet.open ("/sys/devices/system/cpu/cpu"+std::to_string(thread)+"/cpufreq/scaling_max_freq");
						defSet << defaultSettings.maxCoreFreq / 1000;
						defSet.close();
#ifdef HAVE_MSR_SAFE
						write_msr(thread, IA32_ENERGY_PERF_BIAS, defaultSettings.IA32_ENERGY_PERF_BIAS_default);
#endif
					}
				}
			}
		}
	}

	if (activatedUFS)
	{
		if(usedUFS || hardReset)
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

	if (usedPWRCAP || hardReset)
	{
#ifdef HAVE_MSR_SAFE
		for (int die=0; die<SOCKETS_PER_NODE; die++)
		{
			write_msr(mapCoresToSocket[die][0], MSR_PKG_POWER_LIMIT, defaultSettings.defaultPowerCapPKG);
//			write_msr(mapCoresToSocket[die][0], MSR_DRAM_POWER_LIMIT, defaultSettings.defaultPowerCapRAM);
		}
#elif defined HAVE_X86_ADAPT_H

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
			
//			adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_RAM_POWER_LIMIT");
//			x86_adapt_set_setting(fd, adaptRegisterHandler, &defaultSettings.defaultPowerCapRAM);

			if (x86_adapt_put_device(X86_ADAPT_DIE, die))
				std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
		}
#elif defined HAVE_GEOPM_H
		if (hardReset)
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
#endif
	}
	
#ifdef HAVE_NVML_H
	//GPU frequencies
	if (usedGPUDVFS)
	{
		for (int id=0; id<NVML::getGPUsPerNode(); id++)
		{
			if (nvmlDeviceResetApplicationsClocks(NVML::getGPUHandle(id)) != NVML_SUCCESS)
				std::cerr << "NVML ERROR: failed to reset GPU frequency\n";
		}
	}
	
	//GPU power capping limit
	if (usedGPUPWRCAP)
	{
		for (int id=0; id<NVML::getGPUsPerNode(); id++)
		{
			if (nvmlDeviceSetPowerManagementLimit(NVML::getGPUHandle(id), defaultSettings.defaultPowerCapGPU) != NVML_SUCCESS)
				std::cerr << "NVML ERROR: failed to reset GPU power capping limit\n";
		}
	}
#endif
	
}

/**
 * call close functions of the libraries that were used for environment tuning
 **/
void Environment::close()
{
#ifdef HAVE_NVML_H
	nvmlShutdown();
#endif
#ifdef HAVE_X86_ADAPT_H
	x86_adapt_finalize();
#endif
#ifdef HAVE_MSR_CORE_H
	finalize_msr();
#endif
}


