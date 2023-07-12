#include "a64fxwrapper.h"

using namespace meric;

std::map<std::string, unsigned long long int> A64FX::counters;
unsigned long long int A64FX::COUNTERMAX = LLONG_MAX;
std::map<std::string, double> A64FX::units;

#if defined HAVE_LINUX_PERF_EVENT_H && defined __aarch64__
/**
 * Activates monitoring of A64FX energy performance counters
 **/
void A64FX::init(bool detailed)
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
	
	initCountersUnits();
	initMap(counters);
	
	for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
	{
		counters[iterator->first] = setCounter(PERF_TYPE_RAW, counterLabel[iterator->first]);
		if (ioctl(iterator->second, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) != 0)
		{
			std::cerr << "PREFEVENT error when reseting counter\n";
		}
		if (ioctl(iterator->second, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) != 0)
		{
			std::cerr << "PREFEVENT error when starting counting\n";
		}
	}
}

void A64FX::close()
{
	for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
	{
		if (ioctl(iterator->second, PERF_EVENT_IOC_DISABLE, 0) != 0)
		{
			std::cerr << "PREFEVENT error when stoping counting\n";
		}
		if (::close(iterator->second) != 0)
		{
			std::cerr << "PREFEVENT error when closing file descriptor\n";
		}
	}
}

/**
 * Opens file descriptor of the requested performance counters
 **/
long A64FX::openCounter(perf_event_attr attr)
{
	//struct perf_event_attr, pid, cpu, group_fd, flags
	long fd = syscall(__NR_perf_event_open, &attr, syscall(__NR_gettid), -1, -1, 0);
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
		std::cerr << "PREFEVENT error while opening new fd\n"; 
	}
	return fd;
}

#else
void A64FX::init(bool detailed)
{
	return;
}
void A64FX::close()
{
	return;
}
#endif

/**
 * Some counters have alternative label to be presented to user
 * Not a case of the A64FX class
 **/
std::string A64FX::getCounterLabel(std::string counter)
{
	return counter;
}

/**
 * Label representing a sum of values of cumulative counters
 * All A64FX energy counters are cumulative
 **/
std::string A64FX::getSummaryCounterName()
{
	return "Energy consumption [J]";
}

/**
 * Read raw performance counters
 **/
void A64FX::getCounterValues(std::map<std::string, unsigned long long int> & record) //copy-pasted
{
	for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
	{
		unsigned long long int result = 0;
		if ((read(iterator->second, &result, sizeof(unsigned long long int))) != sizeof(unsigned long long int))
		{
			std::cerr << "PREFEVENT error: failed to read counter "<< iterator->first << std::endl;
			record[iterator->first] = 0;
		}
		else
		{
			record[iterator->first] = result;
		}
	}
}

/**
 * Solve counter overflow + transforms performance counters to energy [J]
 **/
double A64FX::getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, int & cumulative, std::string counter, std::string regionName)
{
	cumulative = 1;
	if (!units.count(counter))
	{
		return 0.0;
	}

	if (stopValue < startValue)
	{
		MERIC_INFO << "PERFEVENT OVERFLOW, MAX VALUE: " << COUNTERMAX << "*" << units[counter] << " " <<counter << std::endl;
		MERIC_INFO << "PERFEVENT REPAIR " << startValue << " .. " << stopValue << " = " << ((COUNTERMAX - startValue) + stopValue) * units[counter] << std::endl;
		return ((COUNTERMAX - startValue) + stopValue) * units[counter];
	}
	else
	{
		return (stopValue - startValue) * units[counter];
	}
}

/**
 * Energy counters units [J]
 * Units of A64FX (2.2/2.0/1.8 GHz, 48 cores) are different than units of A64FX (2.6 GHz, 24 cores)
 * https://github.com/fujitsu/A64FX/blob/master/doc/A64FX_PMU_Events_v1.3.pdf
 **/
void A64FX::initCountersUnits()
{
	//nominal frequency of the A64FX is not possible to read from the CPU (it can be measured if a core is under a load from CPU_CYCLES (0x0011) event) so the CPU type identification must be done based on number of cores
	//A64FX has 4 chiplets, while the system identifies each chiplet as a socket
	if (Environment::CORES_PER_SOCKET * 4 == 48)
	{
		units[A64FX_COUNTERS::ENERGY_CORE] = 8e-9;
		units[A64FX_COUNTERS::ENERGY_L2] = 32e-9;
	}
	else
	{
		units[A64FX_COUNTERS::ENERGY_CORE] = 9e-9;
		units[A64FX_COUNTERS::ENERGY_L2] = 36e-9;
	}
	units[A64FX_COUNTERS::ENERGY_MEM] = 256e-9;
}

/**
 * returns unit (double) of the requested counter
 **/
double A64FX::getUnit(std::string counterName)
{
	if (units.count(counterName))
	{
		return units[counterName];
	}
	else
	{
		return -1.0;
	}
}

/**
 * Fills the map with counters to monitor
 **/
void A64FX::initMap(std::map<std::string, unsigned long long int> & dict)
{
	dict[A64FX_COUNTERS::ENERGY_CORE] = 0;
	dict[A64FX_COUNTERS::ENERGY_MEM]  = 0;
	dict[A64FX_COUNTERS::ENERGY_L2]   = 0;
}
