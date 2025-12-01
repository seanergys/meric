#include "a64fxwrapper.h"

using namespace meric;

std::map<const std::string, long> A64FX::countersFd;
unsigned long long int A64FX::COUNTERMAX = LLONG_MAX;
std::map<std::string, double> A64FX::units;

#if defined HAVE_LINUX_PERF_EVENT_H && defined __aarch64__
/**
 * Activates monitoring of A64FX energy performance counters
 **/
void A64FX::init(bool detailed)
{
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
	
	initCountersUnits();
	
	const std::string suffix = "_";
	const int suffixLength = log10(Environment::CORES_PER_SOCKET*Environment::SOCKETS_PER_NODE)+1;
	for (int i=0; i<Environment::CORES_PER_SOCKET*Environment::SOCKETS_PER_NODE; i++)
	{
		std::ostringstream os;
		os << std::setfill('0') << std::setw(suffixLength) << i;
		countersFd[A64FX_COUNTERS::ENERGY_CORE + suffix + os.str()] = setCounter(PERF_TYPE_RAW, counterDefines[A64FX_COUNTERS::ENERGY_CORE], i);
		if (i%Environment::CORES_PER_SOCKET == 0)
		{
			countersFd[A64FX_COUNTERS::ENERGY_L2 + suffix + os.str()] = setCounter(PERF_TYPE_RAW, counterDefines[A64FX_COUNTERS::ENERGY_L2], i);
			countersFd[A64FX_COUNTERS::ENERGY_MEM + suffix + os.str()] = setCounter(PERF_TYPE_RAW, counterDefines[A64FX_COUNTERS::ENERGY_MEM], i);
		}
	}

	for(std::map<std::string, long>::iterator iterator = countersFd.begin(); iterator != countersFd.end(); iterator++)
	{
		if (ioctl(iterator->second, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) != 0)
		{
			std::cerr << "PREFEVENT error when reseting counter\n";
		}
		if (ioctl(iterator->second, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) != 0)
		{
			std::cerr << "PREFEVENT error when starting counting\n";
		}
	}
	
	for(std::map<const std::string, const uint64_t>::iterator iterator = counterDefines.begin(); iterator != counterDefines.end(); iterator++)
	{
		const int pos = A64FX_COUNTERS::counterLabel[iterator->first].find(' ');
		const std::string prefix = A64FX_COUNTERS::counterLabel[iterator->first].substr(0,pos);
		const std::string postfix = A64FX_COUNTERS::counterLabel[iterator->first].substr(pos);
		
		for (int i=0; i<Environment::CORES_PER_SOCKET*Environment::SOCKETS_PER_NODE;)
		{
			std::ostringstream os;
			os << std::setfill('0') << std::setw(suffixLength) << i;
			A64FX_COUNTERS::counterLabel[iterator->first + suffix + os.str()] = prefix + suffix + os.str() + postfix;
			
			if (iterator->first == A64FX_COUNTERS::ENERGY_CORE)
			{
				i++;
			}
			else
			{
				i+=Environment::CORES_PER_SOCKET;
			}
		}
	}
}

void A64FX::close()
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

/**
 * Opens file descriptor of the requested performance counters
 **/
long A64FX::openCounter(perf_event_attr attr, int cpu)
{
	//struct perf_event_attr, pid, cpu, group_fd, flags
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

bool A64FX::isAvailable()
{
#if defined HAVE_LINUX_PERF_EVENT_H && defined __aarch64__
	return true;
#else
	return false;
#endif 
}

/**
 * each counter has a label, that will represent this counter in the output file
 * returns a label of the input counter
 **/
std::string A64FX::getCounterLabel(std::string counter)
{
	std::map<const std::string, std::string>::iterator it = A64FX_COUNTERS::counterLabel.find(counter);
	if (it != A64FX_COUNTERS::counterLabel.end())
	{
		return it->second;
	}
	else
	{
		return counter;
	}
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
void A64FX::getCounterValues(std::map<std::string, unsigned long long int> & record)
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

/**
 * Solve counter overflow + transforms performance counters to energy [J]
 **/
double A64FX::getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, int & cumulative, std::string counter, std::string regionName)
{
	const std::string counterName = counter.substr(0, counter.find_last_of('_'));

	cumulative = 1;
	if (!units.count(counterName))
	{
		return 0.0;
	}

	if (stopValue < startValue)
	{
		MERIC_INFO << "PERFEVENT OVERFLOW, MAX VALUE: " << COUNTERMAX << "*" << units[counterName] << " " <<counterName << std::endl;
		MERIC_INFO << "PERFEVENT REPAIR " << startValue << " .. " << stopValue << " = " << ((COUNTERMAX - startValue) + stopValue) * units[counterName] << std::endl;
		return ((COUNTERMAX - startValue) + stopValue) * units[counterName];
	}
	else
	{
		return (stopValue - startValue) * units[counterName];
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
	//A64FX has 4 NUMA domains, while the system identifies each domain as a socket
	if (Environment::CORES_PER_SOCKET * Environment::SOCKETS_PER_NODE == 48) // XXX XXX XXX XXX XXX XXX XXX XXX
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
