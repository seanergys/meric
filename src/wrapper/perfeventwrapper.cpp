// #define _GNU_SOURCE
#ifdef HAVE_LINUX_PERF_EVENT_H
#include <linux/perf_event.h>
#endif
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
// #include <errno.h>
#include "perfeventwrapper.h"
#include "counters.h"

using namespace meric;

#ifdef HAVE_LINUX_PERF_EVENT_H
std::map<std::string, long> PERFEVENT::counters;

long PERFEVENT::openCounter(perf_event_attr attr)
{
	//(struct perf_event_attr, pid, cpu, group_fd, flags);
	long fd = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
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



void PERFEVENT::init()
{

	auto setCounter = [&] (__u32 type, __u64 config)
	{
		struct perf_event_attr attr;
		memset(&attr, 0, sizeof(struct perf_event_attr));
		attr.size = sizeof(struct perf_event_attr);
		attr.inherit = 1;
		attr.type = type;
		attr.config = config;

		return openCounter(attr);		
	};
	
	counters[PERFEVENT_COUNTERS::COUNTER_AVX] = setCounter(PERF_TYPE_RAW, 0x04C6); //0x04C6 == AVX_INSTS.CALC
	counters[PERFEVENT_COUNTERS::COUNTER_AVX_ALL] = setCounter(PERF_TYPE_RAW, 0x07C6); //0x07C6 == AVX_INSTS.ALL
	counters[PERFEVENT_COUNTERS::COUNTER_CAHE_MISSES] = setCounter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
	counters[PERFEVENT_COUNTERS::COUNTER_HW_INSTRUCTIONS] = setCounter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);

	start();
}

void PERFEVENT::close()
{
	for(std::map<std::string, long>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
	{
		if (ioctl(iterator->second, PERF_EVENT_IOC_DISABLE, 0) != 0)
			std::cerr << "PREFEVENT error when stoping counting\n";
	}
	
	for(std::map<std::string, long>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
	{
		if (::close(iterator->second) != 0)
			std::cerr << "PREFEVENT error when closing file descriptor\n"; 
	}

}


void PERFEVENT::start()
{
	for(std::map<std::string, long>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
	{
		if (ioctl(iterator->second, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) != 0)
			std::cerr << "PREFEVENT error when reseting counter\n";
		if (ioctl(iterator->second, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) != 0)
			std::cerr << "PREFEVENT error when starting counting\n";
	}
}



void PERFEVENT::getValues(std::map<std::string, long> & record) 
{
	for(std::map<std::string, long>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
	{
		long long result = 0;
		if ((read(iterator->second, &result, sizeof(long long))) != sizeof(long long))
		{
			//int errn = errno;
			std::cerr << "PREFEVENT error. Failed to read counter "<< iterator->first << std::endl;		//TODO ? 0
			record[iterator->first] = 0;
		}
		else
			record[iterator->first] = result;
	}
}


#else
#endif

void PERFEVENT::initMap(std::map<std::string, long> & dict)
{
	dict[PERFEVENT_COUNTERS::COUNTER_AVX] = 0;
	dict[PERFEVENT_COUNTERS::COUNTER_AVX_ALL] = 0;
	dict[PERFEVENT_COUNTERS::COUNTER_CAHE_MISSES] = 0;
	dict[PERFEVENT_COUNTERS::COUNTER_HW_INSTRUCTIONS] = 0;
}



