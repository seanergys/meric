#include "msrcounterswrapper.h"
#include "raplwrapper.h"

using namespace meric;

uint64_t MSR::COUNTERMAX = 0xFFFFFFFFFFFFFFFF;
uint64_t MSR::nominal_freq = 0;
CpuidSignatureValues MSR::cpu_arch = CpuidSignatureValues::UNKNOWN;


#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H
std::map<std::string, uint64_t> MSR::counters;

#if defined U_MSR_CBO_0_PMON_CTL0 && defined U_MSR_CBO_0_PMON_CTR0
	#define U_MSR_CBO_x_PMON_CTLy_2_U_MSR_CBO_x_PMON_CTRy(ctrl_addr) ctrl_addr + 0x6 // based on the documentation, ctrl0:0x2002 - ctr0:0x2008, ctrl1:0x2003 - ctr1:0x2009, ctrl2:0x2004 - ctr2:0x200A... etc
	#define U_MSR_CBO_x_PMON_CTRy_2_U_MSR_CBO_x_PMON_CTLy(ctr_addr) ctr_addr - 0x6	 // based on the documentation, ctrl0:0x2002 - ctr0:0x2008, ctrl1:0x2003 - ctr1:0x2009, ctrl2:0x2004 - ctr2:0x200A... etc
#endif
/**
 * @brief Stores and retrieves MSR register addresses along with ownership information 
 *        for uncore frequency reading on Sapphire Rapids and later CPUs.
 *
 * @description This function maintains a list of MSR register addresses paired with 
 * ownership status, applicable only to Sapphire Rapids and newer models. To retrieve 
 * stored values, call the function without arguments. To update stored addresses 
 * and ownership, provide a vector of address-ownership pairs.
 *
 * - Non-zero MSR values update corresponding indices, while zero values leave them unchanged.
 * - Ownership is indicated in the second element of the pair: `0` means not owned, 
 *   while any positive value signifies ownership.
 *
 * @param msr_data A vector of pairs, where each pair consists of:
 *        - First element (`uint32_t`): MSR register address (non-zero updates the stored address).
 *        - Second element (`uint32_t`): Ownership flag (`0` = not owned, `>0` = owned).
 *        Missing or zero entries retain their previous values.
 *
 * @return A vector of pairs, where each pair contains a stored MSR register address 
 *         and its corresponding ownership status.
 *
 * @note This function is defined as `static` and is only callable within this compilation unit.
 *       It dynamically expands to store additional MSR addresses when needed.
 */
static std::vector<std::pair<uint32_t, uint32_t>> intelTiledUncoreCounterMSR(
	const std::vector<std::pair<uint32_t, uint32_t>>& msr_data = {})
{
	//TODO remove SPR from the method name

	static std::vector<std::pair<uint32_t, uint32_t>> counterMsrs;

	// If no parameters are passed, return the current list immediately
	if (msr_data.empty())
	{
		return counterMsrs;
	}

	// Initialize the vector on the first call
	if (counterMsrs.empty())
	{
		counterMsrs.resize(msr_data.size(), {static_cast<uint32_t>(-1), 0}); // Default: -1 (unset MSR), 0 (not owned)
	}

	// Update stored MSR addresses and ownership
	for (size_t i = 0; i < msr_data.size(); ++i)
	{
		if (msr_data[i].first != 0) // Update only if non-zero
		{
			counterMsrs[i].first = msr_data[i].first;
		}
		if (msr_data[i].second > 0) // Update ownership if explicitly set
		{
			counterMsrs[i].second = msr_data[i].second;
		}
	}
	return counterMsrs;
}


void MSR::init()
{
	// msr library as well as RAPL units has been already initialized in Environment::init(...)
	initMap(counters);
#ifdef _INTEL_
	std::string cpuFamilyModelString;
	Environment::getCPUFamilyModelString(cpuFamilyModelString);
	MSR::cpu_arch = meric::stringToCpuidSignature(cpuFamilyModelString);
#if defined U_MSR_PMON_GLOBAL_CTL && defined U_MSR_CBO_0_PMON_CTL0 && defined U_MSR_CBO_59_PMON_CTL0
	if (MSR::cpu_arch >= CpuidSignatureValues::INTEL_06_8FH) // based on Sapphire Rapids microarchitecture or later
	{
		/**
		* ---> if it is freezed, then unfreeze and keep it unfreeze
		*  --> check cboxes to find an available one, if there's none print and warning and not activate
		* ---> when event select is 0x0 then it is available to us.
		* ---> remove freeze operations here (don't freeze only un-freeze)
		*/
		const uint64_t EVENT_CBOX_CLOCKTICKS = 0x1; //Clock ticks of the uncore caching and home agent (CHA)
		const uint64_t EVENT_CBOX_CLOCKTICKS_bits = U_MSR_CBO_x_PMON_CTLx_reset_mask | (1 << U_MSR_CBO_x_PMON_CTLx_thresh_shift) | EVENT_CBOX_CLOCKTICKS;

		std::vector<std::pair<uint32_t, uint32_t>>  msrCTRandOwnership;
		for (int socket = 0; socket < Environment::SOCKETS_PER_NODE; socket++)
		{
			bool socket_free_box_found = false;
			write_msr(Environment::mapCoresToSocket[socket][0], U_MSR_PMON_GLOBAL_CTL, 0x0); // always un-freeze counting on global ctl and never freeze.
			// Traverse CBOXes to find an appropriate one
			for (uint64_t U_MSR_CBO_x_PMON_addr = U_MSR_CBO_0_PMON_CTL0; U_MSR_CBO_x_PMON_addr <= U_MSR_CBO_59_PMON_CTL0; U_MSR_CBO_x_PMON_addr += U_MSR_CBO_x_PMON_CTLx_increment)
			{
				for (uint64_t U_MSR_CBO_x_PMON_CTLy_addr = U_MSR_CBO_x_PMON_addr; U_MSR_CBO_x_PMON_CTLy_addr < (U_MSR_CBO_x_PMON_addr + 0x4); U_MSR_CBO_x_PMON_CTLy_addr += 0x1) //TODO increment
				{
					// check if ctr is available.
					uint64_t U_MSR_CBO_x_PMON_CTLx_bits;
					if (read_msr(Environment::mapCoresToSocket[socket][0], U_MSR_CBO_x_PMON_CTLy_addr, &U_MSR_CBO_x_PMON_CTLx_bits)) // continue traversing if msr is not read (check msr_allowlist for the addr.)
					{
						continue;
					}

					if ((U_MSR_CBO_x_PMON_CTLx_bits & U_MSR_CBO_x_PMON_CTLx_event_select_mask) == 0x0) // if it is empty configure for monitoring (check the event_code section [7:0] bits to determine if it is empty)
					{
						socket_free_box_found = true;
						msrCTRandOwnership.push_back({U_MSR_CBO_x_PMON_CTLy_2_U_MSR_CBO_x_PMON_CTRy(U_MSR_CBO_x_PMON_CTLy_addr), 1});	// add the counter of that controller, we owned the msr.
						write_msr(Environment::mapCoresToSocket[socket][0], U_MSR_CBO_x_PMON_CTLy_addr, EVENT_CBOX_CLOCKTICKS_bits);

						MERIC_INFO << "Socket[" << socket << "]: Empty slot is found, configuring for uncore frequency monitoring.\n";
						break; // exit traversing U_MSR_CBO_x_PMON_CTLy_addr
					}
				}
				if (socket_free_box_found)	// early termination
				{
					break; // Exit the outer loop to check the other socket.
				}
			} 
			if (!socket_free_box_found)	// check if it found a box for the given socket or not.
			{
				msrCTRandOwnership.push_back({-1, 0});		// this is must! since we couldn't find a box for the given socket, we set "-1" to indicate no msr availability and 0 for no ownership.
				MERIC_INFO << "Socket " << socket << ": All CBoxes are occupied, Uncore frequency monitoring is not active.\n";	
			}
		}
		
		auto msr_adresses = intelTiledUncoreCounterMSR(msrCTRandOwnership); // store the found msr counter addresses.
		MERIC_INFO << "Uncore freq monitoring configured to: ";
		for(auto addr : msrCTRandOwnership)
		{
			MERIC_INFO << std::hex << "msr:" << U_MSR_CBO_x_PMON_CTRy_2_U_MSR_CBO_x_PMON_CTLy(addr.first) << " ownership:" << addr.second << "\n";
		}
	}
	else
#endif	
	{
		// enable uncore-frequency monitoring for other architectures
#ifdef U_MSR_PMON_FIXED_CTL
		uint64_t U_MSR_PMON_FIXED_CTL_bits = 1 << U_MSR_PMON_FIXED_CTL_shift;
		for (int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
		{
			write_msr(Environment::mapCoresToSocket[i][0], U_MSR_PMON_FIXED_CTL, U_MSR_PMON_FIXED_CTL_bits);
		}
#endif
		// TODO prepare a msr batch
	}
#endif
	nominal_freq = Environment::getNominalFrequency();
}

void MSR::close()
{
	// library will be finalized in Environment::close()
#ifdef _INTEL_
	if (MSR::cpu_arch >= CpuidSignatureValues::INTEL_06_8FH) // based on Sapphire Rapids microarchitecture or later
	{
		auto msr_adresses = intelTiledUncoreCounterMSR(); // read the found msr counter addresses.
		// to close sequence, never freeze the unit box, clear the control so it will stop.
		for (int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
		{
			if(msr_adresses[i].first != -1 && msr_adresses[i].second == 1)	// there's an msr and we own it
			{
				// reset the ctrl for all zeros + rst bit. since we don't stop any counter, we do this twice to makesure the counter is reset properly
				write_msr(Environment::mapCoresToSocket[i][0], U_MSR_CBO_x_PMON_CTRy_2_U_MSR_CBO_x_PMON_CTLy(msr_adresses[i].first), U_MSR_CBO_x_PMON_CTLx_reset_mask);
				MERIC_INFO << "setting ctrl 0x" << std::hex << U_MSR_CBO_x_PMON_CTLx_reset_mask  << " to MSR " << U_MSR_CBO_x_PMON_CTRy_2_U_MSR_CBO_x_PMON_CTLy(msr_adresses[i].first) << "\n"; 
			}
		}
	}
#endif
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
			auto msr_adresses = intelTiledUncoreCounterMSR(); // read the found msr counter addresses.
			if (i < msr_adresses.size() && msr_adresses[i].first != -1) // read msr if it is correctly set (available box) 
			{
				read_msr(Environment::mapCoresToSocket[i][0], msr_adresses[i].first, &val);
			}
		}
		else
		{
#ifdef U_MSR_PMON_FIXED_CTR
			read_msr(Environment::mapCoresToSocket[i][0], U_MSR_PMON_FIXED_CTR, &val);
#endif
		}

#ifdef _INTEL_
		record[MSR_COUNTERS::UCF_FREQ + "_" + std::to_string(i)] = val;
#endif // _INTEL_

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
	{
		return stopValue - startValue;
	}
}

void MSR::initMap(std::map<std::string, uint64_t> &dict)
{
#ifdef __x86_64
	for (int i = 0; i < Environment::SOCKETS_PER_NODE * Environment::CORES_PER_SOCKET; i++)
	{
		std::string suffix = "_" + std::to_string(i);
		dict[MSR_COUNTERS::APERF + suffix] = 0;
		dict[MSR_COUNTERS::MPERF + suffix] = 0;
#ifdef _INTEL_
		dict[MSR_COUNTERS::CORE_THERM_STATUS + suffix] = 0;
#endif
	}

#ifdef _INTEL_
	for (int i = 0; i < Environment::SOCKETS_PER_NODE; i++)
	{
		std::string suffix = "_" + std::to_string(i);
		dict[MSR_COUNTERS::UCF_FREQ + suffix] = 0;
		dict[MSR_COUNTERS::RAPL_PKG_ACTIVE + suffix] = 0;
		dict[MSR_COUNTERS::PACKAGE_THERM_STATUS + suffix] = 0;
	}
#endif
#endif
}
