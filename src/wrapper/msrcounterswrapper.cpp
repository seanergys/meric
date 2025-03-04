#include "msrcounterswrapper.h"
#include "raplwrapper.h"

using namespace meric;

uint64_t MSR::COUNTERMAX = 0xFFFFFFFFFFFFFFFF;
uint64_t MSR::nominal_freq = 0;
CpuidSignatureValues MSR::cpu_arch = CpuidSignatureValues::UNKNOWN;

//---- undef at the end of the file.
#define cha_ctrl0_traverser(i) for (size_t i = MSR_UNC_SPR_C0_PMON_CTRL; i <= 0x23B2; i += 0x10)
#define ctrl_traverser(i, cha_ctrl0_addr) for (size_t i = cha_ctrl0_addr; i < (cha_ctrl0_addr + 0x4); i += 0x1)
#define ctrl_to_ctr(ctrl_addr) ctrl_addr + 6 // based on the documentation, ctrl0:0x2002 - ctr0:0x2008, ctrl1:0x2003 - ctr1:0x2009, ctrl2:0x2004 - ctr2:0x200A... etc
#define ctr_to_ctrl(ctr_addr) ctr_addr - 6	 // based on the documentation, ctrl0:0x2002 - ctr0:0x2008, ctrl1:0x2003 - ctr1:0x2009, ctrl2:0x2004 - ctr2:0x200A... etc
//----

#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H
std::map<std::string, uint64_t> MSR::counters;

/**
 * @brief Stores and retrieves MSR register addresses for uncore frequency reading for sapphire rapids and later cpus.
 *
 * @description This function maintains the MSR register addresses for uncore frequency measurement,
 * applicable only to Sapphire Rapids and newer models. To retrieve stored values, call the function
 * without arguments. To update the stored addresses, provide the respective MSR values.
 * Use the `ctrl_to_ctr` or `ctr_to_ctrl` utilities for conversion between control and counter registers.
 *
 * @param addr0 The counter MSR register address for socket 0 (default: 0, meaning not valid).
 * @param addr1 The counter MSR register address for socket 1 (default: 0, meaning not valid).
 *
 * @return A pair containing the stored MSR addresses for socket 0 and socket 1.
 *
 * @note This function is defined as `static` and is only callable within this compilation unit.
 */

static std::pair<uint32_t, uint32_t> sapphireRapidsUncoreCounterMSR(uint32_t addr0 = 0, uint32_t addr1 = 0)
{
	static uint32_t counterMsrSocket0 = 0;
	static uint32_t counterMsrSocket1 = 0;

	// If provided values are non-zero, update the stored values
	if (addr0 != 0)
		counterMsrSocket0 = addr0;
	if (addr1 != 0)
		counterMsrSocket1 = addr1; // Fixed incorrect variable assignment

	return {counterMsrSocket0, counterMsrSocket1};
}

void MSR::init()
{
	// msr library as well as RAPL units has been already initialized in Environment::init(...)
	initMap(counters);

	std::string cpuFamilyModelString;
	Environment::getCPUFamilyModelString(cpuFamilyModelString);
	MSR::cpu_arch = meric::stringToCpuidSignature(cpuFamilyModelString);
	if (MSR::cpu_arch >= CpuidSignatureValues::INTEL_06_8FH) // based on Sapphire Rapids microarchitecture or later
	{
		// TODO

		/**
		 *  TODO
		 *  --> check cboxes to find an available one, if there's none print and warning and not activate
		 * ---> when event select is 0x0 (check this) then it is available to us.
		 * ---> if it is freezed, then unfreeze but never freeze yoruself and keep it unfreeze
		 * ---> remove freeze operations here (don't freeze only un-freeze)
		 * --->
		 *
		 */
		uint32_t counterMsrSocket0 = 0;
		uint32_t counterMsrSocket1 = 0;
		for (int socket = 0; socket < Environment::SOCKETS_PER_NODE; socket++)
		{
			std::cout << "----------- socket:" << socket << "-----------\n"; 
			// Traverse CBOXes to find an appropriate one
			cha_ctrl0_traverser(cha_ctrl0_addr)
			{
				std::cout << std::hex << cha_ctrl0_addr << " ---- >";
				ctrl_traverser(ctrl, cha_ctrl0_addr)
				{
					std::cout << std::hex << "(" << ctrl << " " << ctrl_to_ctr(ctrl) << "), ";
					// check if ctr is available.
					uint64_t val;
					read_msr(Environment::mapCoresToSocket[socket][0], ctrl, &val);
					std::cout << std::hex << "ctrl val: " << val << "\n";
					if ((val & 0xFF) == 0x0) // if it is empty configure for monitoring
					{
						(socket == 0 ? counterMsrSocket0 : counterMsrSocket1) = ctrl_to_ctr(ctrl);
						write_msr(Environment::mapCoresToSocket[socket][0], ctrl, 0x1020001); // 0x1020001 to set configuration
					}
					else if ((val & 0xFF) == 0x1) // if it is already monitoring, just read it.
					{
						(socket == 0 ? counterMsrSocket0 : counterMsrSocket1) = ctrl_to_ctr(ctrl);
					}
					return;
				}
				std::cout << "\n";
			}
			// if(false) // check if it is available
			// {

			// }
			// else{	// if there's none, print and error and continue
			// 	MERIC_INFO << "All Cboxes are occupied, Uncore frequency is not being monitored\n";
			// }
			// write_msr(Environment::mapCoresToSocket[i][0], MSR_UNC_SPR_PMON_GLOBAL_CTL, 0x1); // freeze counting by global ctl
			// write_msr(Environment::mapCoresToSocket[i][0], MSR_UNC_SPR_C0_PMON_UNIT_CTRL, 0x300); // 0x300 to clear unit box control and counter values
			// write_msr(Environment::mapCoresToSocket[i][0], MSR_UNC_SPR_C0_PMON_CTRL, 0x1020001); // 0x1020001 to set configuration
			// write_msr(Environment::mapCoresToSocket[i][0], MSR_UNC_SPR_PMON_GLOBAL_CTL, 0x0); // un-freeze counting by global ctl
		}
		if (counterMsrSocket0 == 0)
		{
			MERIC_INFO << "Socket 0: All Cboxes are occupied, Uncore frequency monitoring is not active.\n";
		}

		if (counterMsrSocket1 == 0)
		{
			MERIC_INFO << "Socket 1: All Cboxes are occupied, Uncore frequency monitoring is not active.\n";
		}

		sapphireRapidsUncoreCounterMSR(counterMsrSocket0, counterMsrSocket1); // store the msr adrresses found.
	}
	else
	{
		// enable uncore-frequency monitoring for other architectures
		uint64_t U_MSR_PMON_FIXED_CTL_bits = 1 << U_MSR_PMON_FIXED_CTL_shift;
#ifdef U_MSR_PMON_FIXED_CTL
		for (int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
			write_msr(Environment::mapCoresToSocket[i][0], U_MSR_PMON_FIXED_CTL, U_MSR_PMON_FIXED_CTL_bits);
#endif
		nominal_freq = Environment::getNominalFrequency();
		// TODO prepare a msr batch
	}
}

void MSR::close()
{
	// library will be finalized in Environment::close()
	if (MSR::cpu_arch >= CpuidSignatureValues::INTEL_06_8FH) // based on Sapphire Rapids microarchitecture or later
	{
		// TODO: to close sequence, never freeze, clear the control so it will stop.
		for (int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
		{
			write_msr(Environment::mapCoresToSocket[i][0], MSR_UNC_SPR_PMON_GLOBAL_CTL, 0x1);	  // freeze counting by global ctl
			write_msr(Environment::mapCoresToSocket[i][0], MSR_UNC_SPR_C0_PMON_UNIT_CTRL, 0x300); // 0x300 to clear unit box control and counter values
		}
	}
}

void MSR::getCounterValues(std::map<std::string, uint64_t> &record)
{
	uint64_t reg, val;
	for (int i = 0; i < Environment::SOCKETS_PER_NODE * Environment::CORES_PER_SOCKET; i++)
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

	for (int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		if (MSR::cpu_arch >= CpuidSignatureValues::INTEL_06_8FH)
		{ // based on Sapphire Rapids microarchitecture or later
#ifdef MSR_UNC_SPR_C0_PMON_CTR
			read_msr(Environment::mapCoresToSocket[i][0], MSR_UNC_SPR_C0_PMON_CTR, &val);
#endif
		}
		else
		{
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

void MSR::getRatios(std::map<std::string, uint64_t> &start, std::map<std::string, uint64_t> &stop, double runtime, std::map<std::string, double> &ratios)
{
	int cumulative = 0;
	// frequencies
	int i = 0;
	std::string suffix = "_0";
	while (stop.count(MSR_COUNTERS::APERF + suffix) && stop.count(MSR_COUNTERS::MPERF + suffix))
	{
		ratios["CPU_FREQ" + suffix + " [Hz]"] = CountersRates::CPU_FREQUENCY_FROM_Pstate_PERF(
			getResultValue(start[MSR_COUNTERS::APERF + suffix], stop[MSR_COUNTERS::APERF + suffix], runtime, cumulative),
			getResultValue(start[MSR_COUNTERS::MPERF + suffix], stop[MSR_COUNTERS::MPERF + suffix], runtime, cumulative),
			nominal_freq);
		suffix = "_" + std::to_string(++i);
	}
	i = 0;
	suffix = "_0";
	while (stop.count(MSR_COUNTERS::UCF_FREQ + suffix))
	{
		ratios["CPU_UNCORE_FREQ" + suffix + " [Hz]"] = CountersRates::CPU_UNCORE_FREQUNECY(
			getResultValue(start[MSR_COUNTERS::UCF_FREQ + suffix], stop[MSR_COUNTERS::UCF_FREQ + suffix], runtime, cumulative),
			runtime);
		suffix = "_" + std::to_string(++i);
	}

	// RAPL throttling activity
	i = 0;
	suffix = "_0";
	while (stop.count(MSR_COUNTERS::RAPL_PKG_ACTIVE + suffix))
	{
		ratios["RAPL_PKG_ACTIVE_RATIO" + suffix] = CountersRates::RAPL_ACTIVE_RATIO(
			getResultValue(start[MSR_COUNTERS::RAPL_PKG_ACTIVE + suffix], stop[MSR_COUNTERS::RAPL_PKG_ACTIVE + suffix], runtime, cumulative) * RAPL::getUnit(MSR_COUNTERS::RAPL_PKG_ACTIVE),
			runtime);
		suffix = "_" + std::to_string(++i);
	}

	// temperature
	i = 0;
	suffix = "_0";
	while (stop.count(MSR_COUNTERS::CORE_THERM_STATUS + suffix))
	{
		ratios["START_TEMP_CORE" + suffix + " [degrees C]"] = TCC - start[MSR_COUNTERS::CORE_THERM_STATUS + suffix];
		ratios["STOP_TEMP_CORE" + suffix + " [degrees C]"] = TCC - stop[MSR_COUNTERS::CORE_THERM_STATUS + suffix];
		suffix = "_" + std::to_string(++i);
	}

	i = 0;
	suffix = "_0";
	while (stop.count(MSR_COUNTERS::PACKAGE_THERM_STATUS + suffix))
	{
		ratios["START_TEMP_PKG" + suffix + " [degrees C]"] = TCC - start[MSR_COUNTERS::PACKAGE_THERM_STATUS + suffix];
		ratios["STOP_TEMP_PKG" + suffix + " [degrees C]"] = TCC - stop[MSR_COUNTERS::PACKAGE_THERM_STATUS + suffix];
		suffix = "_" + std::to_string(++i);
	}
}

std::string MSR::getSourceLabel()
{
	return "MSR";
}

std::string MSR::getCounterLabel(std::string counter)
{
	// return MSR_COUNTERS::counterLabel[counter]; // currently there is no counter with a label
	return "";
}

std::string MSR::getSummaryCounterName()
{
	return "";
}

// solve counter overflow
uint64_t MSR::getResultValue(uint64_t startValue, uint64_t stopValue, double runtime, int &cumulative, std::string counter, std::string regionName)
{
	cumulative = 0;
	if (stopValue < startValue)
	{
		MERIC_INFO << "MSR OVERFLOW, MAX VALUE: " << COUNTERMAX << " " << counter << std::endl
				   << "MSR REPAIR " << startValue << " .. " << stopValue << " = " << COUNTERMAX - startValue + stopValue << std::endl;
		return COUNTERMAX - startValue + stopValue;
	}
	else
		return stopValue - startValue;
}

void MSR::initMap(std::map<std::string, uint64_t> &dict)
{
	for (int i = 0; i < Environment::SOCKETS_PER_NODE * Environment::CORES_PER_SOCKET; i++)
	{
		std::string suffix = "_" + std::to_string(i);
		dict[MSR_COUNTERS::APERF + suffix] = 0;
		dict[MSR_COUNTERS::MPERF + suffix] = 0;
		dict[MSR_COUNTERS::CORE_THERM_STATUS + suffix] = 0;
	}

	for (int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		std::string suffix = "_" + std::to_string(i);
		dict[MSR_COUNTERS::UCF_FREQ + suffix] = 0;
		dict[MSR_COUNTERS::RAPL_PKG_ACTIVE + suffix] = 0;
		dict[MSR_COUNTERS::PACKAGE_THERM_STATUS + suffix] = 0;
	}
}

#undef cha_ctrl0_traverser
#undef ctrl_traverser
#undef ctrl_to_ctr