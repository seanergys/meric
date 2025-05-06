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
#endif

#ifdef HAVE_LINUX_PERF_EVENT_H
std::map<const std::string, long> PERFEVENT::countersFd;

void PERFEVENT::init(bool detailed)
{
	detailedMode = detailed;

	#ifdef _AMD_
	initL3CacheSizes();

	__u32 PMU_TYPE_AMD_L3 = getPMUtype("amd_l3");
	#endif

	int CORES_PER_NODE = Environment::CORES_PER_SOCKET*Environment::SOCKETS_PER_NODE;
	int coreIdxLength = log10(CORES_PER_NODE) + 1;

	//Mapping cores to core indexes
	std::vector<int> mapCoresToCoreIdxs;
	mapCoresToCoreIdxs.reserve(CORES_PER_NODE);
	for(int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		for(int j = 0; j < Environment::CORES_PER_SOCKET; j++)
		{
			int core = Environment::mapCoresToSocket[i][j];

			mapCoresToCoreIdxs.push_back(core);
		}
	}

	#ifdef _AMD_
	int L3_CACHES_PER_NODE = PERFEVENT::L3_CACHES_PER_SOCKET*Environment::SOCKETS_PER_NODE;
	int L3CacheIdxLength = log10(L3_CACHES_PER_NODE) + 1;

	//Mapping cores to L3 cache indexes
	std::vector<int> mapCoresToL3CacheIdxs;
	mapCoresToL3CacheIdxs.reserve(L3_CACHES_PER_NODE);
	for(int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		for(int j = 0; j < PERFEVENT::L3_CACHES_PER_SOCKET; j++)
		{
			int L3Cache = PERFEVENT::mapL3CachesToSocket[i][j];
			int core = PERFEVENT::mapCoresToL3Cache[L3Cache][0];

			mapCoresToL3CacheIdxs.push_back(core);
		}
	}
	#endif

	//Initialization of per-core monitoring of CPU cycles and CPU instructions
	for(int i = 0; i < CORES_PER_NODE; i++)
	{
		std::ostringstream idxFill;
		idxFill << std::setfill('0') << std::setw(coreIdxLength) << i;
		std::string suffix = "_" + idxFill.str();

		int core = mapCoresToCoreIdxs[i];
		
		countersFd[PERFEVENT_COUNTERS::PERFEVENT_CPU_CYCLES + suffix] = openCounter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, core);
		countersFd[PERFEVENT_COUNTERS::PERFEVENT_CPU_INSTS + suffix] = openCounter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, core);
	}

	#ifdef _AMD_
	//Initialization of per-core monitoring of supported AMD counters
	for(int i = 0; i < CORES_PER_NODE; i++)
	{
		std::ostringstream idxFill;
		idxFill << std::setfill('0') << std::setw(coreIdxLength) << i;
		std::string suffix = "_" + idxFill.str();

		int core = mapCoresToCoreIdxs[i];
	
		countersFd[PERFEVENT_COUNTERS::AMD_CPU_FLOPS + suffix] = openCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::AMD_CPU_FLOPS], core);
	}

	//Initialization of monitoring of L3 cache misses (AMD)
	for(int i = 0; i < L3_CACHES_PER_NODE; i++)
	{
		std::ostringstream idxFill;
		idxFill << std::setfill('0') << std::setw(L3CacheIdxLength) << i;
		std::string suffix = "_" + idxFill.str();

		int core = mapCoresToL3CacheIdxs[i];

		countersFd[PERFEVENT_COUNTERS::AMD_CPU_L3_MISSES + suffix] = openCounter(PMU_TYPE_AMD_L3, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::AMD_CPU_L3_MISSES], core);
	}
	
	#elif defined _INTEL_
	//Initialization of per-core monitoring of supported Intel counters
	for(int i = 0; i < CORES_PER_NODE; i++)
	{
		std::ostringstream idxFill;
		idxFill << std::setfill('0') << std::setw(coreIdxLength) << i;
		std::string suffix = "_" + idxFill.str();

		int core = mapCoresToCoreIdxs[i];

		//countersFd[PERFEVENT_COUNTERS::INTEL_CPU_CYCLES_AMX_ACTIVE + suffix] = openCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_CYCLES_AMX_ACTIVE], core);
		countersFd[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_SCALAR_FP_DP + suffix] = openCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_SCALAR_FP_DP], core);
		countersFd[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC128_FP_DP + suffix] = openCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC128_FP_DP], core);
		countersFd[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC256_FP_DP + suffix] = openCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC256_FP_DP], core);
		countersFd[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC512_FP_DP + suffix] = openCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC512_FP_DP], core);
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
	for(std::map<const std::string, long>::iterator iterator = countersFd.begin(); iterator != countersFd.end(); iterator++)
	{
		unsigned long long int result = 0;
		if ((read(iterator->second, &result, sizeof(unsigned long long int))) != sizeof(unsigned long long int))
		{
			//int errn = errno;
			std::cerr << "PERFEVENT error - failed to read counter " << iterator->first << "\n";
			record[iterator->first] = 0;
		}
		else
		{
			record[iterator->first] = result;
		}
	}
}

#else
#endif

void PERFEVENT::getRatios(std::map<std::string, unsigned long long int>& start, std::map<std::string, unsigned long long int>& stop, double runtime, std::map <std::string, double> & ratios)
{
	int cumulative = 0;

	int CORES_PER_NODE = Environment::CORES_PER_SOCKET*Environment::SOCKETS_PER_NODE;
	int coreIdxLength = log10(CORES_PER_NODE) + 1;

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

	int L3_CACHES_PER_NODE = PERFEVENT::L3_CACHES_PER_SOCKET*Environment::SOCKETS_PER_NODE;
	int L3CacheIdxLength = log10(L3_CACHES_PER_NODE) + 1;

	//Mapping L3 caches to L3 cache indexes
	std::vector<int> mapL3CachesToL3CacheIdxs;
	mapL3CachesToL3CacheIdxs.reserve(L3_CACHES_PER_NODE);
	for(int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		for(int j = 0; j < PERFEVENT::L3_CACHES_PER_SOCKET; j++)
		{
			int L3Cache = (!PERFEVENT::mapL3CachesToSocket.empty()) ? PERFEVENT::mapL3CachesToSocket[i][j] : i;

			mapL3CachesToL3CacheIdxs.push_back(L3Cache);
		}
	}
	#endif

	//Initialization of map counterValues
	std::map<std::string, std::vector<unsigned long long int>> counterValues;
	counterValues[PERFEVENT_COUNTERS::PERFEVENT_CPU_CYCLES].reserve(CORES_PER_NODE);
	counterValues[PERFEVENT_COUNTERS::PERFEVENT_CPU_INSTS].reserve(CORES_PER_NODE);
	#ifdef _AMD_
	counterValues[PERFEVENT_COUNTERS::AMD_CPU_FLOPS].reserve(CORES_PER_NODE);
	counterValues[PERFEVENT_COUNTERS::AMD_CPU_L3_MISSES].reserve(L3_CACHES_PER_NODE);
	#elif defined _INTEL_
	//counterValues[PERFEVENT_COUNTERS::INTEL_CPU_CYCLES_AMX_ACTIVE].reserve(CORES_PER_NODE);
	counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_SCALAR_FP_DP].reserve(CORES_PER_NODE);
	counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC128_FP_DP].reserve(CORES_PER_NODE);
	counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC256_FP_DP].reserve(CORES_PER_NODE);
	counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC512_FP_DP].reserve(CORES_PER_NODE);
	#endif

	//Construction of map counterValues
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

	for(int i = 0; i < CORES_PER_NODE; i++)
	{
		std::ostringstream idxFill;
		idxFill << std::setfill('0') << std::setw(coreIdxLength) << i;
		std::string suffix = "_" + idxFill.str();

		unsigned long long int cycles = counterValues[PERFEVENT_COUNTERS::PERFEVENT_CPU_CYCLES][i];

		ratios["CPU_CORE_FREQUENCY"+suffix+" [Hz]"] = CountersRates::CPU_FREQUENCY_FROM_CYCLES(cycles, runtime);

		unsigned long long int instructions = counterValues[PERFEVENT_COUNTERS::PERFEVENT_CPU_INSTS][i];

		#ifdef _AMD_
		unsigned long long int flops = counterValues[PERFEVENT_COUNTERS::AMD_CPU_FLOPS][i];

		ratios["CPU_FLOP"+suffix+" [FLOP/s]"] = flops/runtime;
		
		#elif defined _INTEL_
		//unsigned long long int cyclesAMXActive = counterValues[PERFEVENT_COUNTERS::INTEL_CPU_CYCLES_AMX_ACTIVE][i];
		
		//ratios["CPU_AMX_ACTIVE"+suffix+" [%]"] = (cyclesAMXActive/cycles)*100;

		unsigned long long int instructionsScalarFpDP = counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_SCALAR_FP_DP][i];
		unsigned long long int instructionsVec128FpDP = counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC128_FP_DP][i];
		unsigned long long int instructionsVec256FpDP = counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC256_FP_DP][i];
		unsigned long long int instructionsVec512FpDP = counterValues[PERFEVENT_COUNTERS::INTEL_CPU_INSTS_VEC512_FP_DP][i];

		unsigned long long int flops = instructionsScalarFpDP + 2*instructionsVec128FpDP + 4*instructionsVec256FpDP + 8*instructionsVec512FpDP;

		ratios["CPU_FLOP_DP"+suffix+" [FLOP/s]"] = flops/runtime;
		
		unsigned long long int instructionsVecFpDP = instructionsVec128FpDP + instructionsVec256FpDP + instructionsVec512FpDP;
		unsigned long long int instructionsFpDP = instructionsScalarFpDP + instructionsVecFpDP;

		ratios["CPU_INSTRUCTION SCALAR_FP_DP/FP_DP"+suffix+" [%]"] = instructionsFpDP ? (instructionsScalarFpDP/(double)instructionsFpDP)*100 : 0;
		ratios["CPU_INSTRUCTION VEC_128B_FP_DP/FP_DP"+suffix+" [%]"] = instructionsFpDP ? (instructionsVec128FpDP/(double)instructionsFpDP)*100 : 0;
		ratios["CPU_INSTRUCTION VEC_256B_FP_DP/FP_DP"+suffix+" [%]"] = instructionsFpDP ? (instructionsVec256FpDP/(double)instructionsFpDP)*100 : 0;
		ratios["CPU_INSTRUCTION VEC_512B_FP_DP/FP_DP"+suffix+" [%]"] = instructionsFpDP ? (instructionsVec512FpDP/(double)instructionsFpDP)*100 : 0;
		ratios["CPU_INSTRUCTION VEC_FP_DP/FP_DP"+suffix+" [%]"] = instructionsFpDP ? (instructionsVecFpDP/(double)instructionsFpDP)*100 : 0;

		ratios["CPU_INSTRUCTION SCALAR_FP_DP/ALL"+suffix+" [%]"] = instructions ? (instructionsScalarFpDP/(double)instructions)*100 : 0;
		ratios["CPU_INSTRUCTION VEC_128B_FP_DP/ALL"+suffix+" [%]"] = instructions ? (instructionsVec128FpDP/(double)instructions)*100 : 0;
		ratios["CPU_INSTRUCTION VEC_256B_FP_DP/ALL"+suffix+" [%]"] = instructions ? (instructionsVec256FpDP/(double)instructions)*100 : 0;
		ratios["CPU_INSTRUCTION VEC_512B_FP_DP/ALL"+suffix+" [%]"] = instructions ? (instructionsVec512FpDP/(double)instructions)*100 : 0;
		ratios["CPU_INSTRUCTION VEC_FP_DP/ALL"+suffix+" [%]"] = instructions ? (instructionsVecFpDP/(double)instructions)*100 : 0;
		#endif
	}

	#ifdef _AMD_
	for(int i = 0; i < L3_CACHES_PER_NODE; i++)
	{
		std::ostringstream idxFill;
		idxFill << std::setfill('0') << std::setw(L3CacheIdxLength) << i;
		std::string suffix = "_" + idxFill.str();

		unsigned long long int L3CacheMisses = counterValues[PERFEVENT_COUNTERS::AMD_CPU_L3_MISSES][i];
		unsigned long long int instructions = 0;

		if(L3CacheMisses > 0)
		{
			int L3Cache = mapL3CachesToL3CacheIdxs[i];
			int CORES_PER_L3_CACHE = PERFEVENT::mapCoresToL3Cache[L3Cache].size();
			for(int j = 0; j < CORES_PER_L3_CACHE; j++)
			{
				int core = PERFEVENT::mapCoresToL3Cache[L3Cache][j];
				int coreIdx = mapCoreIdxsToCores[core];

				instructions += counterValues[PERFEVENT_COUNTERS::PERFEVENT_CPU_INSTS][coreIdx];
			}
		}

		ratios["CPU_COMPUTATIONAL_INTENSITY"+suffix+" [instructions/B]"] = L3CacheMisses ? instructions/((double)L3CacheMisses*64) : 0;
	}
	#endif
}

std::string PERFEVENT::getSourceLabel()
{
	return "PERFEVENT";
}

std::string PERFEVENT::getCounterLabel(std::string counter)
{
	if (!detailedMode)
	{
		return "";
	}
	
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
		return counter;
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
