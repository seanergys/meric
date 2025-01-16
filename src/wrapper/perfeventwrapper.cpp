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
	
	//counters[PERFEVENT_COUNTERS::COUNTER_AVX] = setCounter(PERF_TYPE_RAW, 0x04C6); //0x04C6 == AVX_INSTS.CALC
	//counters[PERFEVENT_COUNTERS::COUNTER_AVX_ALL] = setCounter(PERF_TYPE_RAW, 0x07C6); //0x07C6 == AVX_INSTS.ALL
	//counters[PERFEVENT_COUNTERS::COUNTER_CACHE_MISSES] = setCounter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);
	//counters[PERFEVENT_COUNTERS::COUNTER_HW_INSTRUCTIONS] = setCounter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
	//counters[PERFEVENT_COUNTERS::COUNTER_FREQ] = setCounter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);

	for (int i=0; i < Environment::CORES_PER_SOCKET*Environment::SOCKETS_PER_NODE; i++)
	{
		std::string suffix = "_" + std::to_string(i);
		
		countersFd[PERFEVENT_COUNTERS::COUNTER_CPU_CYCLES + suffix] = setCounter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, i);
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
	//if (stop.count(PERFEVENT_COUNTERS::COUNTER_AVX))
	//{
	//	ratios["Performance [GFLOPs]"] = CountersRates::FLOPS(
	//		stop[PERFEVENT_COUNTERS::COUNTER_AVX] - 
	//		start[PERFEVENT_COUNTERS::COUNTER_AVX], 
	//		runtime
	//		);			
	//}
	//if (stop.count(PERFEVENT_COUNTERS::COUNTER_CACHE_MISSES) && stop.count(PERFEVENT_COUNTERS::COUNTER_HW_INSTRUCTIONS))
	//{
	//	ratios["Computational Intensity"] = CountersRates::computationalIntensity(
	//			stop[PERFEVENT_COUNTERS::COUNTER_HW_INSTRUCTIONS] - 
	//			start[PERFEVENT_COUNTERS::COUNTER_HW_INSTRUCTIONS], 
	//			stop[PERFEVENT_COUNTERS::COUNTER_CACHE_MISSES] - 
	//			start[PERFEVENT_COUNTERS::COUNTER_CACHE_MISSES]
	//		);
	//}
	//if (stop.count(PERFEVENT_COUNTERS::COUNTER_FREQ))
	//{
	//	ratios["CPU frequency [Hz]"] = CountersRates::CPU_FREQUENCY_FROM_CYCLES(
	//			stop[PERFEVENT_COUNTERS::COUNTER_FREQ] -
	//			start[PERFEVENT_COUNTERS::COUNTER_FREQ],
	//			runtime
	//		);
	//}

	int cumulative = 0;

	int i = 0;
	std::string suffix = "_" + std::to_string(i);
	while(stop.count(PERFEVENT_COUNTERS::COUNTER_CPU_CYCLES + suffix))
	{
		ratios["CPU_FREQ"+suffix+" [Hz]"] = CountersRates::CPU_FREQUENCY_FROM_CYCLES(
			getResultValue(start[PERFEVENT_COUNTERS::COUNTER_CPU_CYCLES + suffix],
			stop[PERFEVENT_COUNTERS::COUNTER_CPU_CYCLES + suffix], runtime, cumulative),
			runtime);

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
//	dict[PERFEVENT_COUNTERS::] = 0;
//#if defined(__x86_64__)
//	dict[PERFEVENT_COUNTERS::COUNTER_AVX] = 0;
//	dict[PERFEVENT_COUNTERS::COUNTER_AVX_ALL] = 0;
//#endif
//	dict[PERFEVENT_COUNTERS::COUNTER_CACHE_MISSES] = 0;
//	dict[PERFEVENT_COUNTERS::COUNTER_HW_INSTRUCTIONS] = 0;
//	dict[PERFEVENT_COUNTERS::COUNTER_FREQ] = 0;

	for (int i=0; i < Environment::SOCKETS_PER_NODE*Environment::CORES_PER_SOCKET; i++)
	{
		std::string suffix = "_" + std::to_string(i);

		dict[PERFEVENT_COUNTERS::COUNTER_CPU_CYCLES + suffix] = 0;
	}
}
