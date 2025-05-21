// #define _GNU_SOURCE
#ifdef HAVE_LINUX_PERF_EVENT_H
#include <linux/perf_event.h>
#endif

#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <limits.h>
// #include <errno.h>

#include "perfeventwrapper.h"
#include "../basis/utils.h"

using namespace meric;

bool PERFEVENT::detailedMode = false;
unsigned long long int PERFEVENT::COUNTERMAX = LLONG_MAX;

#ifdef _AMD_
int PERFEVENT::L3_CACHES_PER_SOCKET = 1;

std::map<int, std::vector<int>> PERFEVENT::mapCoresToL3Cache;
std::map<int, std::vector<int>> PERFEVENT::mapL3CachesToSocket;

//#elif defined _INTEL_
//int PERFEVENT::IMCs_PER_SOCKET = 1;
#endif

#ifdef HAVE_LINUX_PERF_EVENT_H
std::map<const std::string, long> PERFEVENT::countersFd;

void PERFEVENT::init(bool detailed)
{
	detailedMode = detailed;

	#ifdef _AMD_
	initL3CacheSizes();
	//#elif defined _INTEL_
	//initIMCsizes();
	#endif

	//#ifdef _INTEL_
	//std::string cpuFamilyModel;
	//Environment::getCPUFamilyModelString(cpuFamilyModel);
	//CpuidSignatureValues cpuidSignature = stringToCpuidSignature(cpuFamilyModel);
	//#endif

	std::map<std::string, __u32> PMU_TYPES;

	#ifdef _AMD_
	PMU_TYPES["amd_l3"] = getPMUtype("amd_l3");

//	#elif defined _INTEL_
//	PMU_TYPES["uncore_cha_0"] = getPMUtype("uncore_cha_0");
//
//	for(int i = 0; i < PERFEVENT::IMCs_PER_SOCKET; i++)
//	{
//		std::string PMUname = "uncore_imc_" + std::to_string(i);
//		PMU_TYPES[PMUname] = getPMUtype(PMUname);
//	}
//
//	if(cpuidSignature < CpuidSignatureValues::INTEL_06_8FH)
//	{
//		PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_UNCORE_CYCLES] = 0x0000;
//	}
//	else
//	{
//		PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_UNCORE_CYCLES] = 0x0001;
//	}
//
//	if(cpuidSignature < CpuidSignatureValues::INTEL_06_6CH)
//	{
//		PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_DRAM_BYTES_READ] = 0x0304;
//		PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_DRAM_BYTES_WRITE] = 0x0c04;
//	}
//	else if((cpuidSignature >= CpuidSignatureValues::INTEL_06_6CH) && (cpuidSignature < CpuidSignatureValues::INTEL_06_8FH))
//	{
//		PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_DRAM_BYTES_READ] = 0x0f04;
//		PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_DRAM_BYTES_WRITE] = 0x3004;
//	}
//	else
//	{
//		PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_DRAM_BYTES_READ] = 0xcf05;
//		PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_DRAM_BYTES_WRITE] = 0xf005;
//	}
	#endif

	int coreIdxLength = log10(Environment::CORES_PER_SOCKET*Environment::SOCKETS_PER_NODE) + 1;
	#ifdef _AMD_
	int L3CacheIdxLength = log10(PERFEVENT::L3_CACHES_PER_SOCKET*Environment::SOCKETS_PER_NODE) + 1;
	//#elif defined _INTEL_
	//int IMCidxLength = log10(PERFEVENT::IMCs_PER_SOCKET*Environment::SOCKETS_PER_NODE) + 1;
	#endif
	//#ifdef _INTEL_
	//int socketIdxLength = log10(Environment::SOCKETS_PER_NODE) + 1;
	//#endif

	//Initialization of per-core monitoring of CPU cycles and CPU instructions
	for(int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		for(int j = 0; j < Environment::CORES_PER_SOCKET; j++)
		{
			int coreIdx = i*Environment::CORES_PER_SOCKET + j;

			std::ostringstream coreIdxFill;
			coreIdxFill << std::setfill('0') << std::setw(coreIdxLength) << coreIdx;
			std::string suffixCoreIdx = "_" + coreIdxFill.str();

			int core = Environment::mapCoresToSocket[i][j];
			
			countersFd[PERFEVENT_COUNTERS::PERFEVENT_CPU_CYCLES + suffixCoreIdx] = openCounter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, core);
			countersFd[PERFEVENT_COUNTERS::PERFEVENT_CPU_INSTS + suffixCoreIdx] = openCounter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, core);
		}
	}

	#ifdef _AMD_
	//Initialization of monitoring of supported AMD counters
	for(int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		//(per core)
		for(int j = 0; j < Environment::CORES_PER_SOCKET; j++)
		{
			int coreIdx = i*Environment::CORES_PER_SOCKET + j;

			std::ostringstream coreIdxFill;
			coreIdxFill << std::setfill('0') << std::setw(coreIdxLength) << coreIdx;
			std::string suffixCoreIdx = "_" + coreIdxFill.str();

			int core = Environment::mapCoresToSocket[i][j];
	
			countersFd[PERFEVENT_COUNTERS::AMD_CPU_FLOPS + suffixCoreIdx] = openCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::AMD_CPU_FLOPS], core);
		}

		//(per L3 cache)
		for(int j = 0; j < PERFEVENT::L3_CACHES_PER_SOCKET; j++)
		{
			int L3CacheIdx = i*PERFEVENT::L3_CACHES_PER_SOCKET + j;

			std::ostringstream L3CacheIdxFill;
			L3CacheIdxFill << std::setfill('0') << std::setw(L3CacheIdxLength) << L3CacheIdx;
			std::string suffixL3CacheIdx = "_" + L3CacheIdxFill.str();

			int L3Cache = PERFEVENT::mapL3CachesToSocket[i][j];
			int core = PERFEVENT::mapCoresToL3Cache[L3Cache][0];

			__u32 PMU_TYPE = PMU_TYPES["amd_l3"];
			countersFd[PERFEVENT_COUNTERS::AMD_CPU_L3_MISSES + suffixL3CacheIdx] = openCounter(PMU_TYPE, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::AMD_CPU_L3_MISSES], core);
		}
	}
	
	#elif defined _INTEL_
	//Initialization of monitoring of supported Intel counters
	for(int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		//(per core)
		for(int j = 0; j < Environment::CORES_PER_SOCKET; j++)
		{
			int coreIdx = i*Environment::CORES_PER_SOCKET + j;

			std::ostringstream coreIdxFill;
			coreIdxFill << std::setfill('0') << std::setw(coreIdxLength) << coreIdx;
			std::string suffixCoreIdx = "_" + coreIdxFill.str();

			int core = Environment::mapCoresToSocket[i][j];

			//if(cpuidSignature >= CpuidSignatureValues::INTEL_06_8FH)
			//{
			//	countersFd[PERFEVENT_COUNTERS::INTEL_CPU_CYCLES_AMX_ACTIVE + suffixCoreIdx] = openCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_CYCLES_AMX_ACTIVE], core);
			//}
			countersFd[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_SCALAR_FP_DP + suffixCoreIdx] = openCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_SCALAR_FP_DP], core);
			countersFd[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC128_FP_DP + suffixCoreIdx] = openCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC128_FP_DP], core);
			countersFd[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC256_FP_DP + suffixCoreIdx] = openCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC256_FP_DP], core);
			countersFd[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC512_FP_DP + suffixCoreIdx] = openCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC512_FP_DP], core);
			countersFd[PERFEVENT_COUNTERS::INTEL_CPU_L3_MISSES + suffixCoreIdx] = openCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_L3_MISSES], core);
		}

		//(per IMC)
//		for(int j = 0; j < PERFEVENT::IMCs_PER_SOCKET; j++)
//		{
//			int IMCidx = i*PERFEVENT::IMCs_PER_SOCKET + j;
//
//			std::ostringstream IMCidxFill;
//			IMCidxFill << std::setfill('0') << std::setw(IMCidxLength) << IMCidx;
//			std::string suffixIMCidx = "_" + IMCidxFill.str();
//
//			int core = Environment::mapCoresToSocket[i][0];
//
//			std::string PMUname = "uncore_imc_" + std::to_string(j);
//			__u32 PMU_TYPE = PMU_TYPES[PMUname];
//			countersFd[PERFEVENT_COUNTERS::INTEL_CPU_DRAM_BYTES_READ + suffixIMCidx] = openCounter(PMU_TYPE, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_DRAM_BYTES_READ], core);
//			countersFd[PERFEVENT_COUNTERS::INTEL_CPU_DRAM_BYTES_WRITE + suffixIMCidx] = openCounter(PMU_TYPE, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_DRAM_BYTES_WRITE], core);
//		}
//
//		//(per socket)
//		int socketIdx = i;
//
//		std::ostringstream socketIdxFill;
//		socketIdxFill << std::setfill('0') << std::setw(socketIdxLength) << socketIdx;
//		std::string suffixSocketIdx = "_" + socketIdxFill.str();
//
//		int core = Environment::mapCoresToSocket[i][0];
//
//		__u32 PMU_TYPE = PMU_TYPES["uncore_cha_0"];
//		countersFd[PERFEVENT_COUNTERS::INTEL_CPU_UNCORE_CYCLES + suffixSocketIdx] = openCounter(PMU_TYPE, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_UNCORE_CYCLES], core);
	}
	#endif

	start();
}

void PERFEVENT::close()
{
	for(std::map<const std::string, long>::iterator iterator = countersFd.begin(); iterator != countersFd.end(); iterator++)
	{
		if (ioctl(iterator->second, PERF_EVENT_IOC_DISABLE, 0) != 0)
		{
			std::cerr << "PERFEVENT error - failed to disable counter " << iterator->first << "\n";
		}
	}
	
	for(std::map<const std::string, long>::iterator iterator = countersFd.begin(); iterator != countersFd.end(); iterator++)
	{
		if (::close(iterator->second) != 0)
		{
			std::cerr << "PERFEVENT error - failed to close file descriptor for counter " << iterator->first << "\n";
		}
	}
}

#ifdef _AMD_
void PERFEVENT::initL3CacheSizes()
{
	bool errorOccured = false;

	int TMP_L3_CACHES_PER_SOCKET = 1;

	std::map<int, std::vector<int>> tmpMapCoresToL3Cache;
	std::map<int, std::vector<int>> tmpMapL3CachesToSocket;

	for(int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		for(int j = 0; j < Environment::CORES_PER_SOCKET; j++)
		{
			int core = Environment::mapCoresToSocket[i][j];

			std::string pathToL3CacheIdFile = "/sys/devices/system/cpu/cpu"+std::to_string(core)+"/cache/index3/id";

			int L3Cache;

			std::ifstream L3CacheIdFile;
			L3CacheIdFile.open(pathToL3CacheIdFile);
			if(L3CacheIdFile.is_open())
			{
				if(!(L3CacheIdFile >> L3Cache))
				{
					std::cerr << "PERFEVENT error - failed to read file " << pathToL3CacheIdFile << "\n";
					errorOccured = true;
				}

				L3CacheIdFile.close();
			}
			else
			{
				std::cerr << "PERFEVENT error - failed to open file " << pathToL3CacheIdFile << "\n";
				errorOccured = true;
			}

			if(!errorOccured)
			{
				if(!tmpMapCoresToL3Cache.count(L3Cache))
				{
					tmpMapL3CachesToSocket[i].push_back(L3Cache);

					if(TMP_L3_CACHES_PER_SOCKET < tmpMapL3CachesToSocket[i].size())
					{
						TMP_L3_CACHES_PER_SOCKET++;
					}
				}

				tmpMapCoresToL3Cache[L3Cache].push_back(core);
			}
		}
	}

	if(!errorOccured)
	{
		PERFEVENT::L3_CACHES_PER_SOCKET = TMP_L3_CACHES_PER_SOCKET;

		PERFEVENT::mapCoresToL3Cache = std::move(tmpMapCoresToL3Cache);
		PERFEVENT::mapL3CachesToSocket = std::move(tmpMapL3CachesToSocket);
	}
	else
	{
		for(int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
		{
			PERFEVENT::mapL3CachesToSocket[i].push_back(i);
			PERFEVENT::mapCoresToL3Cache[i].push_back(-1);
		}
	}
}

//#elif defined _INTEL_
//void PERFEVENT::initIMCsizes()
//{
//	std::string pathToPMUsDir = "/sys/bus/event_source/devices";
//
//	DIR* PMUsDir = opendir(pathToPMUsDir.c_str());
//	if(PMUsDir != NULL)
//	{
//		int TMP_IMCs_PER_SOCKET = 0;
//
//		dirent* pPMUsDir = readdir(PMUsDir);
//		while(pPMUsDir != NULL)
//		{
//			std::string PMUname = pPMUsDir->d_name;
//			size_t suffixStartPos = PMUname.find_last_of('_');
//			if(suffixStartPos != std::string::npos)
//			{
//				std::string PMUbaseName = PMUname.substr(0, suffixStartPos);
//				if(PMUbaseName == "uncore_imc")
//				{
//					TMP_IMCs_PER_SOCKET++;
//				}
//			}
//
//			pPMUsDir = readdir(PMUsDir);
//		}
//
//		closedir(PMUsDir);
//
//		if(TMP_IMCs_PER_SOCKET > 0)
//		{
//			PERFEVENT::IMCs_PER_SOCKET = TMP_IMCs_PER_SOCKET;
//		}
//		else
//		{
//			std::cerr << "PERFEVENT error - uncore_imc PMUs not found in directory " << pathToPMUsDir << "\n";
//		}
//	}
//	else
//	{
//		std::cerr << "PERFEVENT error - failed to open directory " << pathToPMUsDir << "\n";
//	}
//}
#endif

__u32 PERFEVENT::getPMUtype(std::string PMUname)
{
	std::string pathToPMUtypeFile = "/sys/bus/event_source/devices/"+PMUname+"/type";

	__u32 PMUtype = 0;

	std::ifstream PMUtypeFile;
	PMUtypeFile.open(pathToPMUtypeFile);
	if(PMUtypeFile.is_open())
	{
		if(!(PMUtypeFile >> PMUtype))
		{
			std::cerr << "PERFEVENT error - failed to read file " << pathToPMUtypeFile << "\n";
			PMUtype = 0;
		}

		PMUtypeFile.close();
	}
	else
	{
		std::cerr << "PERFEVENT error - failed to open file " << pathToPMUtypeFile << "\n";
	}

	return PMUtype;
}

long PERFEVENT::openCounter(__u32 type, __u64 config, int cpu)
{
	long fileDescriptor = -1;

	if((type > 0) || ((type == PERF_TYPE_HARDWARE) && ((config == PERF_COUNT_HW_CPU_CYCLES) || (config == PERF_COUNT_HW_INSTRUCTIONS))))
	{
		struct perf_event_attr attr;
		memset(&attr, 0, sizeof(struct perf_event_attr));
		attr.size = sizeof(struct perf_event_attr);
		attr.inherit = 1;
		attr.type = type;
		attr.config = config;
		attr.disabled = 1;

		//(struct perf_event_attr, pid, cpu, group_fd, flags);
		fileDescriptor = syscall(__NR_perf_event_open, &attr, -1, cpu, -1, 0);
		/*
		pid == 0 and cpu == -1
		    This measures the calling process/thread on any CPU. 
		pid == 0 and cpu >= 0
		    This measures the calling process/thread only when running on the specified CPU. 
		pid > 0 and cpu == -1
		    This measures the specified process/thread on any CPU. 
		pid > 0 and cpu >= 0
		    This measures the specified process/thread only when running on the specified CPU. 
		pid == -1 and cpu >= 0
		    This measures all processes/threads on the specified CPU. This requires CAP_SYS_ADMIN capability or a /proc/sys/kernel/perf_event_paranoid value of less than 1. 
		pid == -1 and cpu == -1
		    This setting is invalid and will return an error.
		*/
	}

	if (fileDescriptor < 0)
	{
		//int err = errno;
		std::cerr << "PERFEVENT error - failed to open new file descriptor\n";
	}
	
	return fileDescriptor;
}

void PERFEVENT::start()
{
	for(std::map<const std::string, long>::iterator iterator = countersFd.begin(); iterator != countersFd.end(); iterator++)
	{
		if (ioctl(iterator->second, PERF_EVENT_IOC_RESET, 0) != 0)
		{
			std::cerr << "PERFEVENT error - failed to reset counter " << iterator->first << "\n";
		}
		if (ioctl(iterator->second, PERF_EVENT_IOC_ENABLE, 0) != 0)
		{
			std::cerr << "PERFEVENT error - failed to enable counter " << iterator->first << "\n";
		}
	}
}

void PERFEVENT::getCounterValues(std::map<std::string, unsigned long long int> & record)
{
	std::string counterBaseName = "";

	int counterMultiplier = 1;

	for(std::map<const std::string, long>::iterator iterator = countersFd.begin(); iterator != countersFd.end(); iterator++)
	{
		if(counterBaseName.empty() || (iterator->first.find(counterBaseName) == std::string::npos))
		{
			size_t suffixStartPos = iterator->first.find_last_of('_');
			counterBaseName = iterator->first.substr(0, suffixStartPos);

			std::unordered_map<std::string, int>::iterator counterMultipliersIterator = PERFEVENT_COUNTERS::counterMultipliers.find(counterBaseName);
			if(counterMultipliersIterator != PERFEVENT_COUNTERS::counterMultipliers.end())
			{
				counterMultiplier = counterMultipliersIterator->second;
			}
			else
			{
				counterMultiplier = 1;
			}
		}

		unsigned long long int result = 0;
		if ((read(iterator->second, &result, sizeof(unsigned long long int))) != sizeof(unsigned long long int))
		{
			//int errn = errno;
			std::cerr << "PERFEVENT error - failed to read counter " << iterator->first << "\n";
			record[iterator->first] = 0;
		}
		else
		{
			record[iterator->first] = result*counterMultiplier;
		}
	}
}

#else
#endif

void PERFEVENT::getRatios(std::map<std::string, unsigned long long int>& start, std::map<std::string, unsigned long long int>& stop, double runtime, std::map <std::string, double> & ratios)
{
	int cumulative = 0;

	//#ifdef _INTEL_
	//std::string cpuFamilyModel;
	//Environment::getCPUFamilyModelString(cpuFamilyModel);
	//CpuidSignatureValues cpuidSignature = stringToCpuidSignature(cpuFamilyModel);
	//#endif

	int CORES_PER_NODE = Environment::CORES_PER_SOCKET*Environment::SOCKETS_PER_NODE;
	int coreIdxLength = log10(CORES_PER_NODE) + 1;

	#ifdef _AMD_
	int L3_CACHES_PER_NODE = PERFEVENT::L3_CACHES_PER_SOCKET*Environment::SOCKETS_PER_NODE;
	int L3CacheIdxLength = log10(L3_CACHES_PER_NODE) + 1;

	//#elif defined _INTEL_
	//int IMCs_PER_NODE = PERFEVENT::IMCs_PER_SOCKET*Environment::SOCKETS_PER_NODE;
	//int IMCidxLength = log10(IMCs_PER_NODE) + 1;
	#endif

	//#ifdef _INTEL_
	//int socketIdxLength = log10(Environment::SOCKETS_PER_NODE) + 1;
	//#endif

	//Initialization and construction of map counterValues
	std::map<std::string, std::vector<unsigned long long int>> counterValues;
	counterValues[PERFEVENT_COUNTERS::PERFEVENT_CPU_CYCLES].reserve(CORES_PER_NODE);
	counterValues[PERFEVENT_COUNTERS::PERFEVENT_CPU_INSTS].reserve(CORES_PER_NODE);

	#ifdef _AMD_
	counterValues[PERFEVENT_COUNTERS::AMD_CPU_FLOPS].reserve(CORES_PER_NODE);
	counterValues[PERFEVENT_COUNTERS::AMD_CPU_L3_MISSES].reserve(L3_CACHES_PER_NODE);

	#elif defined _INTEL_
	//if(cpuidSignature >= CpuidSignatureValues::INTEL_06_8FH)
	//{
	//	counterValues[PERFEVENT_COUNTERS::INTEL_CPU_CYCLES_AMX_ACTIVE].reserve(CORES_PER_NODE);
	//}
	counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_SCALAR_FP_DP].reserve(CORES_PER_NODE);
	counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC128_FP_DP].reserve(CORES_PER_NODE);
	counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC256_FP_DP].reserve(CORES_PER_NODE);
	counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC512_FP_DP].reserve(CORES_PER_NODE);
	counterValues[PERFEVENT_COUNTERS::INTEL_CPU_L3_MISSES].reserve(CORES_PER_NODE);
	//counterValues[PERFEVENT_COUNTERS::INTEL_CPU_UNCORE_CYCLES].reserve(Environment::SOCKETS_PER_NODE);
	//counterValues[PERFEVENT_COUNTERS::INTEL_CPU_DRAM_BYTES_READ].reserve(IMCs_PER_NODE);
	//counterValues[PERFEVENT_COUNTERS::INTEL_CPU_DRAM_BYTES_WRITE].reserve(IMCs_PER_NODE);
	#endif

	for(std::map<std::string, std::vector<unsigned long long int>>::iterator iterator = counterValues.begin(); iterator != counterValues.end(); iterator++)
	{
		int counterIdxLength = log10(iterator->second.capacity()) + 1;

		for(int i = 0; i < iterator->second.capacity(); i++)
		{
			std::ostringstream idxFill;
			idxFill << std::setfill('0') << std::setw(counterIdxLength) << i;
			std::string suffix = "_" + idxFill.str();

			unsigned long long int counterValue = 0;
			if(stop.count(iterator->first + suffix))
			{
				counterValue = getResultValue(start[iterator->first + suffix], stop[iterator->first + suffix], runtime, cumulative);
			}

			iterator->second.push_back(counterValue);
		}
	}

	//Evaluation of CPU core frequency
	for(int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		for(int j = 0; j < Environment::CORES_PER_SOCKET; j++)
		{
			int coreIdx = i*Environment::CORES_PER_SOCKET + j;

			std::ostringstream coreIdxFill;
			coreIdxFill << std::setfill('0') << std::setw(coreIdxLength) << coreIdx;
			std::string suffixCoreIdx = "_" + coreIdxFill.str();

			unsigned long long int cycles = counterValues[PERFEVENT_COUNTERS::PERFEVENT_CPU_CYCLES][coreIdx];

			ratios["CPU_CORE_FREQUENCY"+suffixCoreIdx+" [Hz]"] = CountersRates::CPU_FREQUENCY_FROM_CYCLES(cycles, runtime);
		}
	}

	#ifdef _AMD_
	//Mapping core indexes to cores
	std::map<int, int> mapCoreIdxsToCores;
	for(int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		for(int j = 0; j < Environment::CORES_PER_SOCKET; j++)
		{
			int core = Environment::mapCoresToSocket[i][j];
			int coreIdx = i*Environment::CORES_PER_SOCKET + j;

			mapCoreIdxsToCores[core] = coreIdx;
		}
	}

	//Evaluation of ratios from supported AMD counters
	for(int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		for(int j = 0; j < Environment::CORES_PER_SOCKET; j++)
		{
			int coreIdx = i*Environment::CORES_PER_SOCKET + j;

			std::ostringstream coreIdxFill;
			coreIdxFill << std::setfill('0') << std::setw(coreIdxLength) << coreIdx;
			std::string suffixCoreIdx = "_" + coreIdxFill.str();

			unsigned long long int flops = counterValues[PERFEVENT_COUNTERS::AMD_CPU_FLOPS][coreIdx];

			ratios["CPU_FLOP"+suffixCoreIdx+" [FLOP/s]"] = flops/runtime;
		}

		for(int j = 0; j < PERFEVENT::L3_CACHES_PER_SOCKET; j++)
		{
			int L3CacheIdx = i*PERFEVENT::L3_CACHES_PER_SOCKET + j;

			std::ostringstream L3CacheIdxFill;
			L3CacheIdxFill << std::setfill('0') << std::setw(L3CacheIdxLength) << L3CacheIdx;
			std::string suffixL3CacheIdx = "_" + L3CacheIdxFill.str();

			unsigned long long int L3CacheMisses = counterValues[PERFEVENT_COUNTERS::AMD_CPU_L3_MISSES][L3CacheIdx];

			unsigned long long int instructionsPerL3Cache = 0;
			if(L3CacheMisses > 0)
			{
				int L3Cache = PERFEVENT::mapL3CachesToSocket[i][j];
				int CORES_PER_L3_CACHE = PERFEVENT::mapCoresToL3Cache[L3Cache].size();
				for(int k = 0; k < CORES_PER_L3_CACHE; k++)
				{
					int core = PERFEVENT::mapCoresToL3Cache[L3Cache][k];
					int coreIdx = mapCoreIdxsToCores[core];

					instructionsPerL3Cache += counterValues[PERFEVENT_COUNTERS::PERFEVENT_CPU_INSTS][coreIdx];
				}
			}

			ratios["CPU_COMPUTATIONAL_INTENSITY"+suffixL3CacheIdx+" [instructions/B]"] = L3CacheMisses ? instructionsPerL3Cache/((double)L3CacheMisses*64) : 0;
		}
	}

	#elif defined _INTEL_
	//Evaluation of ratios from supported Intel counters
	for(int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		for(int j = 0; j < Environment::CORES_PER_SOCKET; j++)
		{
			int coreIdx = i*Environment::CORES_PER_SOCKET + j;

			std::ostringstream coreIdxFill;
			coreIdxFill << std::setfill('0') << std::setw(coreIdxLength) << coreIdx;
			std::string suffixCoreIdx = "_" + coreIdxFill.str();

//			if(cpuidSignature >= CpuidSignatureValues::INTEL_06_8FH)
//			{
//				unsigned long long int cyclesAMXActive = counterValues[PERFEVENT_COUNTERS::INTEL_CPU_CYCLES_AMX_ACTIVE][coreIdx];
//				unsigned long long int cycles = counterValues[PERFEVENT_COUNTERS::PERFEVENT_CPU_CYCLES][coreIdx];
//		
//				ratios["CPU_AMX_ACTIVE"+suffixCoreIdx+" [%]"] = cycles ? (cyclesAMXActive/cycles)*100 : 0;
//			}

			unsigned long long int instructionsScalarFpDP = counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_SCALAR_FP_DP][coreIdx];
			unsigned long long int instructionsVec128FpDP = counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC128_FP_DP][coreIdx];
			unsigned long long int instructionsVec256FpDP = counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC256_FP_DP][coreIdx];
			unsigned long long int instructionsVec512FpDP = counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC512_FP_DP][coreIdx];

			unsigned long long int flops = instructionsScalarFpDP + 2*instructionsVec128FpDP + 4*instructionsVec256FpDP + 8*instructionsVec512FpDP;

			ratios["CPU_FLOP_DP"+suffixCoreIdx+" [FLOP/s]"] = flops/runtime;
			
			unsigned long long int instructionsVecFpDP = instructionsVec128FpDP + instructionsVec256FpDP + instructionsVec512FpDP;
			unsigned long long int instructionsFpDP = instructionsScalarFpDP + instructionsVecFpDP;

			ratios["CPU_INSTRUCTION SCALAR_FP_DP/FP_DP"+suffixCoreIdx+" [%]"] = instructionsFpDP ? (instructionsScalarFpDP/(double)instructionsFpDP)*100 : 0;
			ratios["CPU_INSTRUCTION VEC_128B_FP_DP/FP_DP"+suffixCoreIdx+" [%]"] = instructionsFpDP ? (instructionsVec128FpDP/(double)instructionsFpDP)*100 : 0;
			ratios["CPU_INSTRUCTION VEC_256B_FP_DP/FP_DP"+suffixCoreIdx+" [%]"] = instructionsFpDP ? (instructionsVec256FpDP/(double)instructionsFpDP)*100 : 0;
			ratios["CPU_INSTRUCTION VEC_512B_FP_DP/FP_DP"+suffixCoreIdx+" [%]"] = instructionsFpDP ? (instructionsVec512FpDP/(double)instructionsFpDP)*100 : 0;
			ratios["CPU_INSTRUCTION VEC_FP_DP/FP_DP"+suffixCoreIdx+" [%]"] = instructionsFpDP ? (instructionsVecFpDP/(double)instructionsFpDP)*100 : 0;

			unsigned long long int instructions = counterValues[PERFEVENT_COUNTERS::PERFEVENT_CPU_INSTS][coreIdx];

			ratios["CPU_INSTRUCTION SCALAR_FP_DP/ALL"+suffixCoreIdx+" [%]"] = instructions ? (instructionsScalarFpDP/(double)instructions)*100 : 0;
			ratios["CPU_INSTRUCTION VEC_128B_FP_DP/ALL"+suffixCoreIdx+" [%]"] = instructions ? (instructionsVec128FpDP/(double)instructions)*100 : 0;
			ratios["CPU_INSTRUCTION VEC_256B_FP_DP/ALL"+suffixCoreIdx+" [%]"] = instructions ? (instructionsVec256FpDP/(double)instructions)*100 : 0;
			ratios["CPU_INSTRUCTION VEC_512B_FP_DP/ALL"+suffixCoreIdx+" [%]"] = instructions ? (instructionsVec512FpDP/(double)instructions)*100 : 0;
			ratios["CPU_INSTRUCTION VEC_FP_DP/ALL"+suffixCoreIdx+" [%]"] = instructions ? (instructionsVecFpDP/(double)instructions)*100 : 0;

			unsigned long long int L3CacheMisses = counterValues[PERFEVENT_COUNTERS::INTEL_CPU_L3_MISSES][coreIdx];

			ratios["CPU_COMPUTATIONAL_INTENSITY"+suffixCoreIdx+" [instructions/B]"] = L3CacheMisses ? instructions/((double)L3CacheMisses*64) : 0;
		}

//		int socketIdx = i;
//
//		std::ostringstream socketIdxFill;
//		socketIdxFill << std::setfill('0') << std::setw(socketIdxLength) << socketIdx;
//		std::string suffixSocketIdx = "_" + socketIdxFill.str();
//
//		unsigned long long int uncoreCycles = counterValues[PERFEVENT_COUNTERS::INTEL_CPU_UNCORE_CYCLES][socketIdx];
//
//		ratios["CPU_UNCORE_FREQUENCY"+suffixSocketIdx+" [Hz]"] = uncoreCycles/runtime;
//
//		for(int j = 0; j < PERFEVENT::IMCs_PER_SOCKET; j++)
//		{
//			int IMCidx = i*PERFEVENT::IMCs_PER_SOCKET + j;
//
//			unsigned long long int DRAMbytesRead = counterValues[PERFEVENT_COUNTERS::INTEL_CPU_DRAM_BYTES_READ][IMCidx];
//			unsigned long long int DRAMbytesWrite = counterValues[PERFEVENT_COUNTERS::INTEL_CPU_DRAM_BYTES_WRITE][IMCidx];
//
//			if(detailedMode)
//			{
//				std::ostringstream IMCidxFill;
//				IMCidxFill << std::setfill('0') << std::setw(IMCidxLength) << IMCidx;
//				std::string suffixIMCidx = "_" + IMCidxFill.str();
//
//				ratios["CPU_DRAM_BANDWIDTH_READ"+suffixIMCidx+" [B/s]"] = DRAMbytesRead/runtime;
//				ratios["CPU_DRAM_BANDWIDTH_WRITE"+suffixIMCidx+" [B/s]"] = DRAMbytesWrite/runtime;
//				ratios["CPU_DRAM_BANDWIDTH"+suffixIMCidx+" [B/s]"] = (DRAMbytesRead + DRAMbytesWrite)/runtime;
//			}
//			else
//			{
//				ratios["CPU_DRAM_BANDWIDTH_READ"+suffixSocketIdx+" [B/s]"] += DRAMbytesRead/runtime;
//				ratios["CPU_DRAM_BANDWIDTH_WRITE"+suffixSocketIdx+" [B/s]"] += DRAMbytesWrite/runtime;
//				ratios["CPU_DRAM_BANDWIDTH"+suffixSocketIdx+" [B/s]"] += (DRAMbytesRead + DRAMbytesWrite)/runtime;
//			}
//		}
	}
	#endif
}

std::string PERFEVENT::getSourceLabel()
{
	return "PERFEVENT";
}

std::string PERFEVENT::getCounterLabel(std::string counter)
{
	size_t suffixStartPos = counter.find_last_of('_');
	size_t suffixLength = counter.size() - suffixStartPos;
	std::string counterBaseName = counter.substr(0, suffixStartPos);

	std::map<const std::string, std::string>::iterator iterator = PERFEVENT_COUNTERS::counterLabel.find(counterBaseName);
	if (iterator != PERFEVENT_COUNTERS::counterLabel.end())
	{
		return iterator->second + counter.substr(suffixStartPos, suffixLength);
	}
	else
	{
		return "";
	}
}

std::string PERFEVENT::getSummaryCounterName()
{
	return "";
}

// solve counter overflow
unsigned long long PERFEVENT::getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, int & cumulative, std::string counter, std::string regionName)
{
	cumulative = 0;
	if (stopValue < startValue)
	{
		MERIC_INFO << "PERFEVENT OVERFLOW, MAX VALUE: " << COUNTERMAX <<" " <<counter << std::endl;
		MERIC_INFO << "PERFEVENT REPAIR " << startValue << " .. " << stopValue << " = " << COUNTERMAX- startValue + stopValue << std::endl;
		return COUNTERMAX - startValue + stopValue;
	}
	else
	{
		return stopValue - startValue;
	}
}

void PERFEVENT::initMap(std::map<std::string, unsigned long long int> & dict)
{
	for (int i=0; i < Environment::SOCKETS_PER_NODE*Environment::CORES_PER_SOCKET; i++)
	{
		std::string suffix = "_" + std::to_string(i);

		dict[PERFEVENT_COUNTERS::PERFEVENT_CPU_CYCLES + suffix] = 0;
	}
}
