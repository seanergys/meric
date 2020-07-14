#include "msrcounterswrapper.h"
#include "raplwrapper.h"
//#include "../basis/utils.h"

using namespace meric;

uint64_t MSR::COUNTERMAX = 0xFFFFFFFFFFFFFFFF;
uint64_t MSR::nominal_freq = 0;

#ifdef HAVE_MSR_CORE_H
std::map<std::string, uint64_t> MSR::counters;

void MSR::init()
{
	//msr library as well as RAPL units has been already initialized in Environment::init(...)
	initMap(counters);

	//enable uncore-frequency monitoring
	#ifdef HAVE_MSR_CORE_H
	uint64_t U_MSR_PMON_FIXED_CTL_bits = 1 << U_MSR_PMON_FIXED_CTL_shift;
	for (int i=0; i < Environment::SOCKETS_PER_NODE; i++)
		write_msr_by_idx(Environment::mapCoresToSocket[i][0], U_MSR_PMON_FIXED_CTL, U_MSR_PMON_FIXED_CTL_bits);
	#endif

	nominal_freq = Environment::getNominalFrequency();

	//TODO prepare a msr batch
}

void MSR::close()
{
	//library will be finalized in Environment::close()
}

void MSR::getCounterValues(std::map<std::string, uint64_t> & record)
{
	uint64_t reg, val;
	for (int i=0; i < Environment::SOCKETS_PER_NODE * Environment::CORES_PER_SOCKET; i++)
	{
		reg = MSR_COUNTERS::counterLabel[MSR_COUNTERS::APERF];
		read_msr_by_idx(i, reg, &val);
		record[MSR_COUNTERS::APERF + "_" + std::to_string(i)] = val;

		reg = MSR_COUNTERS::counterLabel[MSR_COUNTERS::MPERF];
		read_msr_by_idx(i, reg, &val);
		record[MSR_COUNTERS::MPERF + "_" + std::to_string(i)] = val;
	}
	
	for (int i=0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		reg = MSR_COUNTERS::counterLabel[MSR_COUNTERS::UCF_FREQ];
		read_msr_by_idx(Environment::mapCoresToSocket[i][0], reg, &val);
		record[MSR_COUNTERS::UCF_FREQ + "_" + std::to_string(i)] = val;
		
		reg = MSR_COUNTERS::counterLabel[MSR_COUNTERS::RAPL_PKG_ACTIVE];
		read_msr_by_idx(Environment::mapCoresToSocket[i][0], reg, &val);
		record[MSR_COUNTERS::RAPL_PKG_ACTIVE + "_" + std::to_string(i)] = val;
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
	//RAPL_PKG_ACTIVE
	i = 0;
	suffix = "_0";
	while(stop.count(MSR_COUNTERS::RAPL_PKG_ACTIVE + suffix))
	{
		ratios["RAPL_PKG_ACTIVE_RATIO"+suffix] = CountersRates::RAPL_ACTIVE_RATIO(
			getResultValue(start[MSR_COUNTERS::RAPL_PKG_ACTIVE + suffix], stop[MSR_COUNTERS::RAPL_PKG_ACTIVE + suffix], runtime, "")* RAPL::getUnit(MSR_COUNTERS::RAPL_PKG_ACTIVE),
			runtime);
		suffix = "_" + std::to_string(++i);
	}
}

std::string MSR::getSourceLabel()
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
	for (int i=0; i < Environment::SOCKETS_PER_NODE * Environment::CORES_PER_SOCKET; i++)
	{
		std::string suffix = "_" + std::to_string(i);
		dict[MSR_COUNTERS::APERF + suffix] = 0;
		dict[MSR_COUNTERS::MPERF + suffix] = 0;
	}
	
	for (int i=0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		dict[MSR_COUNTERS::UCF_FREQ+ "_" + std::to_string(i)] = 0;
		dict[MSR_COUNTERS::RAPL_PKG_ACTIVE+ "_" + std::to_string(i)] = 0;
	}
}



