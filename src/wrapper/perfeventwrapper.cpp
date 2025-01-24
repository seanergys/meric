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

#ifdef HAVE_LINUX_PERF_EVENT_H
bool PERFEVENT::detailedMode = false;
std::map<const std::string, long> PERFEVENT::countersFd;
unsigned long long int PERFEVENT::COUNTERMAX = LLONG_MAX;

void PERFEVENT::init(bool detailed)
{
	detailedMode = detailed;

	auto setCounter = [&] (__u32 type, __u64 config, int cpu)
	{
		struct perf_event_attr attr;
		memset(&attr, 0, sizeof(struct perf_event_attr));
		attr.size = sizeof(struct perf_event_attr);
		attr.inherit = 1;
		attr.type = type;
		attr.config = config;
		attr.disabled = 1;

		return openCounter(attr, cpu);
	};
	
	for (int i=0; i < Environment::CORES_PER_SOCKET*Environment::SOCKETS_PER_NODE; i++)
	{
		std::string suffix = "_" + std::to_string(i);
		
		countersFd[PERFEVENT_COUNTERS::COUNTER_CPU_CYCLES + suffix] = setCounter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, i);

		#ifdef _INTEL_
		countersFd[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_SCALAR + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_SCALAR], i);
		countersFd[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC128_PS + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC128_PS], i);
		countersFd[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC128_PD + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC128_PD], i);
		countersFd[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC256_PS + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC256_PS], i);
		countersFd[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC256_PD + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC256_PD], i);
		countersFd[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC512_PS + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC512_PS], i);
		countersFd[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC512_PD + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC512_PD], i);
		//countersFd[PERFEVENT_COUNTERS::COUNTER_CYCLES_AMX_BUSY + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::COUNTER_CYCLES_AMX_BUSY], i);
		#endif
	}

	start();
}

void PERFEVENT::close()
{
	for(std::map<const std::string, long>::iterator iterator = countersFd.begin(); iterator != countersFd.end(); iterator++)
	{
		if (ioctl(iterator->second, PERF_EVENT_IOC_DISABLE, 0) != 0)
		{
			std::cerr << "PREFEVENT error when stoping counting\n";
		}
	}
	
	for(std::map<const std::string, long>::iterator iterator = countersFd.begin(); iterator != countersFd.end(); iterator++)
	{
		if (::close(iterator->second) != 0)
		{
			std::cerr << "PREFEVENT error when closing file descriptor\n";
		}
	}
}

long PERFEVENT::openCounter(perf_event_attr attr, int cpu)
{
	//(struct perf_event_attr, pid, cpu, group_fd, flags);
	long fd = syscall(__NR_perf_event_open, &attr, -1, cpu, -1, 0);
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
	if (fd < 0)
	{
		//int err = errno;
		std::cerr << "PREFEVENT error while opening new fd\n";
	}
	
	return fd;
}

void PERFEVENT::start()
{
	for(std::map<const std::string, long>::iterator iterator = countersFd.begin(); iterator != countersFd.end(); iterator++)
	{
		if (ioctl(iterator->second, PERF_EVENT_IOC_RESET, 0) != 0)
		{
			std::cerr << "PREFEVENT error when reseting counter\n";
		}
		if (ioctl(iterator->second, PERF_EVENT_IOC_ENABLE, 0) != 0)
		{
			std::cerr << "PREFEVENT error when starting counting\n";
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
			std::cerr << "PREFEVENT error. Failed to read counter "<< iterator->first << std::endl;
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

	int i = 0;
	std::string suffix = "_" + std::to_string(i);
	while(stop.count(PERFEVENT_COUNTERS::COUNTER_CPU_CYCLES + suffix))
	{
		unsigned long long int cycles = getResultValue(start[PERFEVENT_COUNTERS::COUNTER_CPU_CYCLES + suffix],
									stop[PERFEVENT_COUNTERS::COUNTER_CPU_CYCLES + suffix], runtime, cumulative);

		ratios["CPU_FREQ"+suffix+" [Hz]"] = CountersRates::CPU_FREQUENCY_FROM_CYCLES(cycles, runtime);

		#ifdef _INTEL_
		unsigned long long int instructionsScalar = getResultValue(start[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_SCALAR + suffix],
									stop[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_SCALAR + suffix], runtime, cumulative);
		
		unsigned long long int instructionsVec128Single = getResultValue(start[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC128_PS + suffix],
									stop[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC128_PS + suffix], runtime, cumulative);
		unsigned long long int instructionsVec128Double = getResultValue(start[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC128_PD + suffix],
									stop[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC128_PD + suffix], runtime, cumulative);

		unsigned long long int instructionsVec256Single = getResultValue(start[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC256_PS + suffix], 
									stop[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC256_PS + suffix], runtime, cumulative);		
		unsigned long long int instructionsVec256Double = getResultValue(start[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC256_PD + suffix], 
									stop[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC256_PD + suffix], runtime, cumulative);

		unsigned long long int instructionsVec512Single = getResultValue(start[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC512_PS + suffix], 
									stop[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC512_PS + suffix], runtime, cumulative);
		unsigned long long int instructionsVec512Double = getResultValue(start[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC512_PD + suffix], 
									stop[PERFEVENT_COUNTERS::COUNTER_INSTS_RET_VEC512_PD + suffix], runtime, cumulative);
		
		unsigned long long int instructionsVec128 = instructionsVec128Single + instructionsVec128Double;
		unsigned long long int instructionsVec256 = instructionsVec256Single + instructionsVec256Double;
		unsigned long long int instructionsVec512 = instructionsVec512Single + instructionsVec512Double;

		unsigned long long int instructionsVector = instructionsVec128 + instructionsVec256 + instructionsVec512;

		unsigned long long int instructionsAll = instructionsScalar + instructionsVector;

		unsigned long long int flops = instructionsScalar +
									4*instructionsVec128Single + 2*instructionsVec128Double +
									8*instructionsVec256Single + 4*instructionsVec256Double +
									16*instructionsVec512Single + 8*instructionsVec512Double;

		ratios["CPU_FLOPS"+suffix+" [FLOPs]"] = flops/runtime;
		ratios["CPU_INSTRUCTION_SCALAR_RATIO"+suffix] = instructionsAll ? instructionsScalar/(double)instructionsAll : 0;
		ratios["CPU_INSTRUCTION_VEC128_RATIO"+suffix] = instructionsAll ? instructionsVec128/(double)instructionsAll : 0;
		ratios["CPU_INSTRUCTION_VEC256_RATIO"+suffix] = instructionsAll ? instructionsVec256/(double)instructionsAll : 0;
		ratios["CPU_INSTRUCTION_VEC512_RATIO"+suffix] = instructionsAll ? instructionsVec512/(double)instructionsAll : 0;
		ratios["CPU_INSTRUCTION_VECTORIZATION_RATIO"+suffix] = instructionsAll ? instructionsVector/(double)instructionsAll : 0;

		//unsigned long long int cyclesAMXBusy = getResultValue(start[PERFEVENT_COUNTERS::COUNTER_CYCLES_AMX_BUSY + suffix],
		//							stop[PERFEVENT_COUNTERS::COUNTER_CYCLES_AMX_BUSY + suffix], runtime, cumulative);
		
		//ratios["CPU_AMX_ACTIVE"+suffix] = cyclesAMXBusy/cycles;
		#endif

		suffix = "_" + std::to_string(++i);
	}
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

		dict[PERFEVENT_COUNTERS::COUNTER_CPU_CYCLES + suffix] = 0;
	}
}
