#include "msrcounterswrapper.h"
#include "raplwrapper.h"

using namespace meric;

uint64_t MSR::COUNTERMAX = 0xFFFFFFFFFFFFFFFF;
uint64_t MSR::nominal_freq = 0;
CpuidSignatureValues MSR::cpu_arch = CpuidSignatureValues::UNKNOWN;

#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H
std::map<std::string, uint64_t> MSR::counters;

void MSR::init()
{
	//msr library as well as RAPL units has been already initialized in Environment::init(...)
	initMap(counters);

	std::string cpuFamilyModelString;
	Environment::getCPUFamilyModelString(cpuFamilyModelString);
	MSR::cpu_arch = meric::stringToCpuidSignature(cpuFamilyModelString);
	if(MSR::cpu_arch >= CpuidSignatureValues::INTEL_06_8FH)
	{
		for (int i=0; i < Environment::SOCKETS_PER_NODE; i++){
			write_msr(Environment::mapCoresToSocket[i][0], MSR_UNC_SPR_PMON_GLOBAL_CTL, 0x1); // freeze counting by global ctl
			write_msr(Environment::mapCoresToSocket[i][0], MSR_UNC_SPR_C0_PMON_UNIT_CTRL, 0x300); // 0x300 to clear unit box control and counter values
			write_msr(Environment::mapCoresToSocket[i][0], MSR_UNC_SPR_C0_PMON_CTRL, 0x1020001); // 0x1020001 to set configuration
			write_msr(Environment::mapCoresToSocket[i][0], MSR_UNC_SPR_PMON_GLOBAL_CTL, 0x0); // un-freeze counting by global ctl
		}
	}
	else
	{
		//enable uncore-frequency monitoring for other architectures
		uint64_t U_MSR_PMON_FIXED_CTL_bits = 1 << U_MSR_PMON_FIXED_CTL_shift;
		#ifdef U_MSR_PMON_FIXED_CTL
		for (int i=0; i < Environment::SOCKETS_PER_NODE; i++)
			write_msr(Environment::mapCoresToSocket[i][0], U_MSR_PMON_FIXED_CTL, U_MSR_PMON_FIXED_CTL_bits);
		#endif
		nominal_freq = Environment::getNominalFrequency();
		//TODO prepare a msr batch
	} 
}

void MSR::close()
{
	//library will be finalized in Environment::close()
	if(MSR::cpu_arch >= CpuidSignatureValues::INTEL_06_8FH)
	{
		for (int i=0; i < Environment::SOCKETS_PER_NODE; i++){
			write_msr(Environment::mapCoresToSocket[i][0], MSR_UNC_SPR_PMON_GLOBAL_CTL, 0x1); // freeze counting by global ctl
			write_msr(Environment::mapCoresToSocket[i][0], MSR_UNC_SPR_C0_PMON_UNIT_CTRL, 0x300); // 0x300 to clear unit box control and counter values
		}
	}
}

void MSR::getCounterValues(std::map<std::string, uint64_t> & record)
{
	uint64_t reg, val;
	for (int i=0; i < Environment::SOCKETS_PER_NODE * Environment::CORES_PER_SOCKET; i++)
	{
#ifdef IA32_APERF
		read_msr(i, IA32_APERF, &val);
		record[MSR_COUNTERS::APERF + "_" + std::to_string(i)] = val;
#endif
#ifdef IA32_MPERF
		read_msr(i, IA32_MPERF, &val);
		record[MSR_COUNTERS::MPERF + "_" + std::to_string(i)] = val;
#endif
#ifdef IA32_THERM_STATUS
		read_msr(i, IA32_THERM_STATUS, &val);
		record[MSR_COUNTERS::CORE_THERM_STATUS + "_" + std::to_string(i)] = (val & IA32_THERM_STATUS_readout_mask) >> IA32_THERM_STATUS_readout_shift;
#endif
	}
	
	for (int i=0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		if(MSR::cpu_arch >= CpuidSignatureValues::INTEL_06_8FH){
#ifdef MSR_UNC_SPR_C0_PMON_CTR
			read_msr(Environment::mapCoresToSocket[i][0], MSR_UNC_SPR_C0_PMON_CTR, &val);
#endif
		}
		else{
#ifdef U_MSR_PMON_FIXED_CTR
			read_msr(Environment::mapCoresToSocket[i][0], U_MSR_PMON_FIXED_CTR, &val);
#endif
		}
		
		record[MSR_COUNTERS::UCF_FREQ + "_" + std::to_string(i)] = val;
		
#ifdef MSR_PKG_PERF_STATUS
		read_msr(Environment::mapCoresToSocket[i][0], MSR_PKG_PERF_STATUS, &val);
		record[MSR_COUNTERS::RAPL_PKG_ACTIVE + "_" + std::to_string(i)] = val;
#endif
#ifdef IA32_PACKAGE_THERM_STATUS
		read_msr(Environment::mapCoresToSocket[i][0], IA32_PACKAGE_THERM_STATUS, &val);
		record[MSR_COUNTERS::PACKAGE_THERM_STATUS + "_" + std::to_string(i)] = (val & IA32_PACKAGE_THERM_STATUS_readout_mask) >> IA32_PACKAGE_THERM_STATUS_readout_shift;
#endif
	}
}

#else
#endif

void MSR::getRatios(std::map<std::string, uint64_t>& start, std::map<std::string, uint64_t>& stop, double runtime, std::map <std::string, double> & ratios)
{
	int cumulative = 0;
	//frequencies
	int i = 0;
	std::string suffix = "_0";
	while(stop.count(MSR_COUNTERS::APERF + suffix) && stop.count(MSR_COUNTERS::MPERF + suffix))
	{
		ratios["CPU_FREQ"+suffix+" [Hz]"] = CountersRates::CPU_FREQUENCY_FROM_Pstate_PERF(
			getResultValue(start[MSR_COUNTERS::APERF + suffix], stop[MSR_COUNTERS::APERF + suffix], runtime, cumulative), 
			getResultValue(start[MSR_COUNTERS::MPERF + suffix], stop[MSR_COUNTERS::MPERF + suffix], runtime, cumulative), 
			nominal_freq);
		suffix = "_" +std::to_string(++i);
	}
	i = 0;
	suffix = "_0";
	while(stop.count(MSR_COUNTERS::UCF_FREQ + suffix))
	{
		ratios["CPU_UNCORE_FREQ"+suffix+" [Hz]"] = CountersRates::CPU_UNCORE_FREQUNECY(
			getResultValue(start[MSR_COUNTERS::UCF_FREQ + suffix], stop[MSR_COUNTERS::UCF_FREQ + suffix], runtime, cumulative),
			runtime);
		suffix = "_" + std::to_string(++i);
	}
	
	//RAPL throttling activity
	i = 0;
	suffix = "_0";
	while(stop.count(MSR_COUNTERS::RAPL_PKG_ACTIVE + suffix))
	{
		ratios["RAPL_PKG_ACTIVE_RATIO"+suffix] = CountersRates::RAPL_ACTIVE_RATIO(
			getResultValue(start[MSR_COUNTERS::RAPL_PKG_ACTIVE + suffix], stop[MSR_COUNTERS::RAPL_PKG_ACTIVE + suffix], runtime, cumulative)* RAPL::getUnit(MSR_COUNTERS::RAPL_PKG_ACTIVE),
			runtime);
		suffix = "_" + std::to_string(++i);
	}
	
	//temperature
	i = 0;
	suffix = "_0";
	while(stop.count(MSR_COUNTERS::CORE_THERM_STATUS + suffix))
	{
		ratios["START_TEMP_CORE"+suffix+" [degrees C]"] = TCC - start[MSR_COUNTERS::CORE_THERM_STATUS + suffix];
		ratios["STOP_TEMP_CORE"+suffix+" [degrees C]"] = TCC - stop[MSR_COUNTERS::CORE_THERM_STATUS + suffix];
		suffix = "_" + std::to_string(++i);
	}

	i = 0;
	suffix = "_0";
	while(stop.count(MSR_COUNTERS::PACKAGE_THERM_STATUS + suffix))
	{
		ratios["START_TEMP_PKG"+suffix+" [degrees C]"] = TCC - start[MSR_COUNTERS::PACKAGE_THERM_STATUS + suffix];
		ratios["STOP_TEMP_PKG"+suffix+" [degrees C]"] = TCC - stop[MSR_COUNTERS::PACKAGE_THERM_STATUS + suffix];
		suffix = "_" + std::to_string(++i);
	}
}

std::string MSR::getSourceLabel()
{
	return "MSR";
}

std::string MSR::getCounterLabel(std::string counter)
{
	//return MSR_COUNTERS::counterLabel[counter]; // currently there is no counter with a label
	return "";
}

std::string MSR::getSummaryCounterName()
{
	return "";
}

// solve counter overflow
uint64_t MSR::getResultValue(uint64_t startValue, uint64_t stopValue, double runtime, int & cumulative, std::string counter, std::string regionName)
{
	cumulative = 0;
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
		dict[MSR_COUNTERS::CORE_THERM_STATUS+suffix] = 0;
	}
	
	for (int i=0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		std::string suffix = "_" + std::to_string(i);
		dict[MSR_COUNTERS::UCF_FREQ + suffix] = 0;
		dict[MSR_COUNTERS::RAPL_PKG_ACTIVE + suffix] = 0;
		dict[MSR_COUNTERS::PACKAGE_THERM_STATUS + suffix] = 0;
	}
}



