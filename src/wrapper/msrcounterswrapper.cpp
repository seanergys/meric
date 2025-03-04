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
 * @brief Stores and retrieves MSR register addresses for uncore frequency reading for Sapphire Rapids and later CPUs.
 *
 * @description This function maintains a list of MSR register addresses for uncore frequency measurement, 
 * applicable only to Sapphire Rapids and newer models. To retrieve stored values, call the function 
 * without arguments. To update the stored addresses, provide a vector of MSR values. Non-zero values 
 * update corresponding indices, while zero values leave them unchanged.
 *
 * @param addrs A vector containing MSR register addresses. Non-zero values update the stored values, 
 * while missing or zero entries retain their previous values.
 *
 * @return A vector containing the stored MSR addresses.
 *
 * @note This function is defined as `static` and is only callable within this compilation unit.
 * It dynamically expands to store additional MSR addresses when needed.
 */

static std::vector<uint32_t> sapphireRapidsUncoreCounterMSR(const std::vector<uint32_t>& addrs = {})
{
    static std::vector<uint32_t> counterMsrs;

    // If no parameters are passed, return the current list immediately
    if (addrs.empty()) {
        return counterMsrs;
    }

    // Initialize the vector to match the input size if it's the first call
    if (counterMsrs.empty()) {
        counterMsrs.resize(addrs.size(), -1);	// set -1 to indicate no msr is set.
    }

    // Update stored values if non-zero values are provided
    for (size_t i = 0; i < addrs.size(); ++i) {
        if (addrs[i] != 0) {
            if (i < counterMsrs.size()) {
                counterMsrs[i] = addrs[i];
            } else {
                counterMsrs.push_back(addrs[i]);
            }
        }
    }

    return counterMsrs;
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

		std::vector<uint32_t> addrs;
		for (int socket = 0; socket < Environment::SOCKETS_PER_NODE; socket++)
		{
			bool socket_free_box_found = false;
			write_msr(Environment::mapCoresToSocket[socket][0], MSR_UNC_SPR_PMON_GLOBAL_CTL, 0x0); // always un-freeze counting on global ctl and never freeze.

			// Traverse CBOXes to find an appropriate one
			cha_ctrl0_traverser(cha_ctrl0_addr)
			{
				ctrl_traverser(ctrl, cha_ctrl0_addr)
				{
					// check if ctr is available.
					uint64_t val = -1; // all 0xFFFFFFFFF...F
					if (read_msr(Environment::mapCoresToSocket[socket][0], ctrl, &val))
						continue;

					if ((val & 0x0F) == 0x0) // if it is empty configure for monitoring (check the event_code section to determine if it is empty)
					{
						socket_free_box_found = true;
						addrs.push_back(ctrl_to_ctr(ctrl));
						write_msr(Environment::mapCoresToSocket[socket][0], ctrl, 0x1020001); // 0x1020001 to set configuration

						MERIC_INFO << "Socket[" << socket << "]: Empty slot is found, configuring for uncore frequency measurements.\n";
						break; // exit traversing ctrl
					}
					else if ((val & 0xFF) == 0x1) // if it is already monitoring, just read it. (https://perfmon-events.intel.com/index.html?pltfrm=spxeon.html&evnt=UNC_CHA_CLOCKTICKS)
					{
						socket_free_box_found = true;
						addrs.push_back(ctrl_to_ctr(ctrl));
						MERIC_INFO << "Socket[" << socket << "]: The event is already being monitored; eavesdropping on the existing monitoring process.\n";
						break; // exit traversing ctrl
					}
				}
				if (socket_free_box_found)	// early termination
					break; // Exit the outer loop to check the other socket.
			} 
			if (!socket_free_box_found){
				addrs.push_back(-1);
				MERIC_INFO << "Socket " << socket << ": All Cboxes are occupied, Uncore frequency monitoring is not active.\n";	
			}
		}
		
		auto msr_adresses = sapphireRapidsUncoreCounterMSR(addrs); // store the found msr counter adrresses.
		MERIC_INFO << "SPR uncore freq configured to: ";
		
		for(uint32_t addr : addrs)
			std::cout << std::hex << addr << ", ";
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
		auto msr_adresses = sapphireRapidsUncoreCounterMSR(); // read the found msr counter adrresses.
		// TODO: to close sequence, never freeze, clear the control so it will stop.
		for (int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
		{
			if(msr_adresses[i] != -1)
				write_msr(Environment::mapCoresToSocket[i][0], ctr_to_ctrl(msr_adresses[i]), 0x300); // 0x300 to clear unit box control and counter values
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
		if (MSR::cpu_arch >= CpuidSignatureValues::INTEL_06_8FH) // based on Sapphire Rapids microarchitecture or later
		{
			auto msr_adresses = sapphireRapidsUncoreCounterMSR(); // read the found msr counter adrresses.
			if (i < msr_adresses.size() && msr_adresses[i] != -1)	// read msr if it is correctly set (available box) 
				read_msr(Environment::mapCoresToSocket[i][0], msr_adresses[i], &val);
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