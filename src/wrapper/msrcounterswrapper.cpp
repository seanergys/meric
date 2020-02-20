#include "msrcounterswrapper.h"
#include "../basis/utils.h"

using namespace meric;

uint64_t MSR::COUNTERMAX = 0xFFFFFFFFFFFFFFFF;
uint64_t MSR::nominal_freq = 0;
uint64_t MSR::CORES_PER_SOCKET = 1;
uint64_t MSR::SOCKETS_PER_NODE = 1;

#ifdef HAVE_MSR_CORE_H
std::map<std::string, uint64_t> MSR::counters;

void MSR::init()
{
	
	#ifdef HAVE_MSR_CORE_H
	core_config(&CORES_PER_SOCKET, NULL, &SOCKETS_PER_NODE, NULL);
	#endif
	
	//msr library has been already initialized in Environment::init(...)
	initMap(counters);

	//enable uncore-frequency monitoring
	#ifdef HAVE_MSR_CORE_H
	uint64_t U_MSR_PMON_FIXED_CTL_bits = 1 << U_MSR_PMON_FIXED_CTL_shift;
	for (int i=0; i < SOCKETS_PER_NODE; i++)
		write_msr_by_idx(i, U_MSR_PMON_FIXED_CTL, U_MSR_PMON_FIXED_CTL_bits);
	#endif

	//TODO prepare a msr batch
	
	uint64_t MSR_PLATFORM_INFO_bits;
	read_msr_by_idx(0, MSR_PLATFORM_INFO, &MSR_PLATFORM_INFO_bits);
	nominal_freq = ((MSR_PLATFORM_INFO_bits & MSR_PLATFORM_INFO_nominal_freq_mask) >> MSR_PLATFORM_INFO_nominal_freq_shift) * 100000000;
}

void MSR::close()
{
	//library will be finalized in Environment::close()
}

void MSR::getCounterValues(std::map<std::string, uint64_t> & record)
{
	for(std::map<std::string, uint64_t>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
	{
		size_t pos = iterator->first.find_last_of("_");
		uint64_t val = 0;
		uint64_t reg = MSR_COUNTERS::counterLabel[iterator->first.substr(0,pos)];
		read_msr_by_idx(std::stoi(iterator->first.substr(pos+1)), reg, &val);
		record[iterator->first] = val;
	}
}

#else
#endif

void MSR::getRatios(std::map<std::string, uint64_t>& start, std::map<std::string, uint64_t>& stop, double runtime, std::map <std::string, double> & ratios)
{
	int i = 0;
	std::string suffix = "_0";
	while(stop.count(MSR_COUNTERS::APERF + suffix) && stop.count(MSR_COUNTERS::MPERF + suffix))
	{
		ratios["CPU_FREQ"+suffix+" [Hz]"] = CountersRates::CPU_FREQUENCY_FROM_Pstate_PERF(
			getResultValue(start[MSR_COUNTERS::APERF + suffix], stop[MSR_COUNTERS::APERF + suffix], runtime, ""), 
			getResultValue(start[MSR_COUNTERS::MPERF + suffix], stop[MSR_COUNTERS::MPERF + suffix], runtime, ""), 
			nominal_freq);
		suffix = "_" +std::to_string(++i);
	}
	i = 0;
	suffix = "_0";
	while(stop.count(MSR_COUNTERS::UCF_FREQ + suffix))
	{
		ratios["CPU_UNCORE_FREQ"+suffix+" [Hz]"] = CountersRates::CPU_UNCORE_FREQUNECY(
			getResultValue(start[MSR_COUNTERS::UCF_FREQ + suffix], stop[MSR_COUNTERS::UCF_FREQ + suffix], runtime, ""),
			runtime);
		suffix = "_" + std::to_string(++i);
	}
}

std::string MSR::getCountersLabel()
{
	return "MSR";
}

std::string MSR::getCounterLabel(std::string counter)
{
	return counter;
}

std::string MSR::getSummaryCounterName()
{
	return "";
}


void MSR::getSummaryValue(double time, unsigned long long int * counterSummary)
{
	*counterSummary = ULLONG_MAX;
}

// solve counter overflow
uint64_t MSR::getResultValue(uint64_t startValue, uint64_t stopValue, double runtime, std::string counter)
{
	if (stopValue < startValue)
	{
		MERIC_INFO << "MSR OVERFLOW, MAX VALUE: " << COUNTERMAX <<" " <<counter << std::endl
		           << "MSR REPAIR " << startValue << " .. " << stopValue << " = " << COUNTERMAX- startValue + stopValue << std::endl;
		return COUNTERMAX - startValue + stopValue;
	}
	else
		return stopValue - startValue;
}

void MSR::initMap(std::map<std::string, uint64_t> & dict)
{
	for (int i=0; i < SOCKETS_PER_NODE * CORES_PER_SOCKET; i++)
	{
		std::string suffix = "_" + std::to_string(i);
		dict[MSR_COUNTERS::APERF + suffix] = 0;
		dict[MSR_COUNTERS::MPERF + suffix] = 0;
	}
	
	for (int i=0; i < SOCKETS_PER_NODE; i++)
		dict[MSR_COUNTERS::UCF_FREQ+ "_" + std::to_string(i)] = 0;
}



