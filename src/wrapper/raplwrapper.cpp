#include "raplwrapper.h"
#include "../basis/utils.h"

/**
 * Intel RAPL energy consumption mearurement registers description
 * From Intel 64 and IA-32 Architectures Software Developer's Manual
 *
 * Intel_RAPL_Pckg_Energy - all energy consumptions
 * Intel_RAPL_PP0_Energy  - CPU
 * Intel_RAPL_PP1_Energy  - may reflect to uncore devices
 * Intel_RAPL_RAM_Energy  - RAM
 *
 * Intel_RAPL_Pckg_Energy ~ Intel_RAPL_PP0_Energy + Intel_RAPL_PP1_Energy
 * Intel_RAPL_Pckg_Energy ~ Intel_RAPL_PP0_Energy + Intel_RAPL_RAM_Energy
 * 
 *
 * MSR_RAPL_POWER_UNIT Register ***********************************************
 * Power Units (bits 3:0):
 	Power related information (in Watts) is based on the multiplier, 1/2^PU; 
 	where PU is an unsigned integer represented by bits 3:0. 
 	Default value is 0011b, indicating power unit is in 1/8 Watts 
increment.
 * Energy Status Units (bits 12:8): 
 	Energy related information (in Joules) is based on the multiplier, 1/2^ESU; 
 	where ESU is an unsigned integer represented by bits 12:8. 
 	Default value is 10000b, indicating energy status unit is 15.3uJ increment.
 * Time Units (bits 19:16): 
	Time related information (in Seconds) is based on the multiplier, 1/2^TU; 
	where TU is an unsigned integer represented by bits 19:16. 
	Default value is 1010b, indicating time unit is in 976us increment.

TIME    ENERGY     POWER
1010|000|01110|0000|0011
9876 543 21098 7654 3210

 * MSR_PKG_ENERGY_STATUS register *********************************************
 * Total energy consumed (bits 31:0):
	The unsigned integer value represents the total amount of energy consumed 
	since that last time this register is cleared. The unit of this field 
	is specified by the "Energy Status Units" field of MSR_RAPL_POWER_UNIT.
 * Reserved (bits 63:32)
 
 * MSR_PP0_ENERGY_STATUS/MSR_PP1_ENERGY_STATUS register ***********************
 * Total Energy Consumed (bits 31:0): 
	The unsigned integer value represents the total amount of energy consumed 
	since the last time this register was cleared. The unit of this field 
	is specified by the "Energy Status Units" field of MSR_RAPL_POWER_UNIT.
 * Reserved (bits 63:32)

 * MSR_DRAM_ENERGY_STATUS register ********************************************
 * Total Energy Consumed (bits 31:0):
	The unsigned integer value represents the total amount of energy consumed 
	since that last time this register is cleared. The unit of this field 
	is specified by the "Energy Status Units" field of MSR_RAPL_POWER_UNIT.
 * Reserved (bits 63:32)
 *
 **/
const uint64_t MSR_RAPL_POWER_UNIT_power_mask   = 0xF;
const uint64_t MSR_RAPL_POWER_UNIT_energy_mask  = 0x1F00;
const uint64_t MSR_RAPL_POWER_UNIT_energy_shift = 8;
const uint64_t MSR_RAPL_POWER_UNIT_time_mask    = 0xF0000;
const uint64_t MSR_RAPL_POWER_UNIT_time_shift   = 16;
//TODO to simplyfy the code, only PKG mask is used in the following code since 
//it is the same as DRAM/PP0/PP1 mask
const uint64_t MSR_PKG_ENERGY_STATUS_mask       = 0xFFFFFFFF;
//const uint64_t MSR_DRAM_ENERGY_STATUS_mask    = 0xFFFFFFFF;
//const uint64_t MSR_PP0_ENERGY_STATUS_mask     = 0xFFFFFFFF;
//const uint64_t MSR_PP1_ENERGY_STATUS_mask     = 0xFFFFFFFF;

#ifdef HAVE_X86_ADAPT_H
	#include "x86_adapt.h"
	using namespace X86ADAPT_RAPL_COUNTERS;
#elif defined (HAVE_MSR_CORE_H)
	#include <msr_core.h>
	#include <msr_rapl.h>
//	#include <master.h>
//	#include <libmsr_error.h>
	using namespace MSR_RAPL_COUNTERS;
#else
	using namespace X86ADAPT_RAPL_COUNTERS;
#endif

using namespace meric;

std::string RAPL::suffix = "_";
int RAPL::NODE_SIZE_SOCKET = 1;
int RAPL::NODE_SIZE_CPU = 1;
int RAPL::SOCKET_SIZE_CPU = 1;
bool RAPL::detailedMode = false;
unsigned long long int RAPL::COUNTERMAX = pow(2,32);

#if defined (HAVE_X86_ADAPT_H) || defined (HAVE_MSR_CORE_H)
std::map<std::string, unsigned long long int> RAPL::counters;
std::map<std::string, double> RAPL::energyUnit;

// solve counter overflow and add baseline energy consumption
unsigned long long RAPL::getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, std::string counter)
{
	if (counter == RAPL_SUM_ENERGY)
		return 0;
	
	std::string counterName;
	if (detailedMode)
		counterName = counter.substr(0,counter.length() - suffix.length() - 1);
	else
		counterName = counter;
	
	if (stopValue < startValue)
	{	
		if (energyUnit.count(counterName))
		{
			MERIC_INFO << "RAPL OVERFLOW, MAX VALUE: " <<COUNTERMAX << "*"<< energyUnit[counterName] <<" " <<counterName << std::endl;
			MERIC_INFO << "RAPL REPAIR " << startValue << " .. " << stopValue << " = " << ((COUNTERMAX - startValue) + stopValue) * energyUnit[counterName] << std::endl;
			return ((COUNTERMAX - startValue) + stopValue) * energyUnit[counterName];
		} //else: unit modifier is missing
		return 0;
	}
	else
		return (stopValue - startValue) * energyUnit[counterName];
}
#endif

#ifdef HAVE_X86_ADAPT_H
// set counters units
void RAPL::init()
{
	NODE_SIZE_SOCKET = x86_adapt_get_nr_avaible_devices(X86_ADAPT_DIE);
	initMap(counters);

	//set unit modifier for Intel_RAPL_Pckg_Energy
	uint64_t unit;
	//ignoring the fact that different sockets may have different units
	const int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
	if (fd < 0)
	{
		std::cerr << "X86ADAPT RAPL INIT error while accessing file descriptor\n";
		std::cerr << "WARNING: RAPL values will be without any unit\n";
		return;
	}
	const int itemId = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_Power_Unit");
	x86_adapt_get_setting(fd, itemId, &unit);
	unit &= MSR_RAPL_POWER_UNIT_energy_mask;
	unit >>= MSR_RAPL_POWER_UNIT_energy_shift;
	if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
		std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
	
	energyUnit[RAPL_PCKG_ENERGY] = 1.0/pow(2,unit);
	//the rest unit modifiers are set according to x86_adapt "documentation"
	//https://github.com/tud-zih-energy/x86_energy/blob/eac130973b2fbfa0b5a3e72ef9912af617584d97/rapl.c#L763
	//DRAM ENERGY unit is fixed to 15.3uJ
	//https://www.intel.com/content/dam/www/public/us/en/documents/datasheets/xeon-e5-v3-datasheet-vol-2.pdf
	energyUnit[RAPL_RAM_ENERGY] = 1.0/pow(2,16);
/*
	energyUnit[RAPL_DRAM_CH0]   = 1.0/pow(2,16);	//1.0/pow(2,18)
	energyUnit[RAPL_DRAM_CH1]   = energyUnit[RAPL_DRAM_CH0];
	energyUnit[RAPL_DRAM_CH2]   = energyUnit[RAPL_DRAM_CH0];
	energyUnit[RAPL_DRAM_CH3]   = energyUnit[RAPL_DRAM_CH0];
*/
}

//	read counters values
void RAPL::getCounterValues(std::map<std::string, unsigned long long int> & record)
{

	for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
		record[iterator->first] = 0;
	record[RAPL_SUM_ENERGY] = 0; //will be filled when the measurement is over
	
	if (detailedMode)
	{
		for (int die=0; die<NODE_SIZE_SOCKET; die++)
		{
			uint64_t dieEnergy = 0;
			const int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, die);
			if (fd < 0)
			{
				std::cerr << "X86ADAPT RAPL error while accessing file descriptor\n";
//				exit(2);
			}
		
			for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
			{
//TODO this version doesn't expect more than 10 sockets per node (should be enough for this century)
				if ((iterator->first.back()-'0') != die)
					continue;
				std::string itemName = iterator->first.substr(0,iterator->first.length() - suffix.length() - 1);
				const int itemId = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, itemName.c_str());
				if (itemId < 0) //== -ENXIO
				{
					std::cerr << "X86ADAPT RAPL wrong counter name - " << itemName <<std::endl;
					continue;
				}
				if (!energyUnit.count(itemName))
				{
					std::cerr << "X86ADAPT RAPL missing unit modifier for counter - " << itemName <<" Add modifier in RAPL::init()"<<std::endl;
					continue;					
				}
				x86_adapt_get_setting(fd, itemId, &dieEnergy);
				record [itemName+suffix+std::to_string(die)] = (dieEnergy & MSR_PKG_ENERGY_STATUS_mask);
			}
			if (x86_adapt_put_device(X86_ADAPT_DIE, die))
				std::cerr << "X86ADAPT RAPL error while closing file descriptor\n";
		}
	}else
	{
		for (int die=0; die<NODE_SIZE_SOCKET; die++)
		{
			uint64_t dieEnergy = 0;
			const int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, die);
			if (fd < 0)
			{
				std::cerr << "X86ADAPT RAPL error while accessing file descriptor\n";
//				exit(2);
			}
			for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
			{
				const int itemId = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, iterator->first.c_str());
				x86_adapt_get_setting(fd, itemId, &dieEnergy);
				record [iterator->first] += (dieEnergy & MSR_PKG_ENERGY_STATUS_mask);
			}
			if (x86_adapt_put_device(X86_ADAPT_DIE, die))
				std::cerr << "X86ADAPT RAPL error while closing file descriptor\n";
		}	
	}
}

////////////////////////////////////////////////////////////////////////////////
#elif  defined HAVE_MSR_CORE_H
// set counters units
void RAPL::init()
{
	uint64_t cs = 0;
	uint64_t ss = 0;
	core_config(&cs, NULL, &ss, NULL);
	NODE_SIZE_SOCKET = ss;
	SOCKET_SIZE_CPU  = cs;
	NODE_SIZE_CPU    = SOCKET_SIZE_CPU * NODE_SIZE_SOCKET;
	initMap(counters);

	uint64_t unit;
	read_msr_by_idx(0, MSR_RAPL_POWER_UNIT, &unit);
	energyUnit[RAPL_PCKG_ENERGY] = 1.0/pow(2,((unit & MSR_RAPL_POWER_UNIT_energy_mask) >> MSR_RAPL_POWER_UNIT_energy_shift));
	//DRAM ENERGY unit is fixed to 15.3uJ
	//https://www.intel.com/content/dam/www/public/us/en/documents/datasheets/xeon-e5-v3-datasheet-vol-2.pdf
	energyUnit[RAPL_RAM_ENERGY] = 1.0/pow(2,16);
}

void RAPL::getCounterValues(std::map<std::string, unsigned long long int> & record)
{
	for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
		record[iterator->first] = 0;
	record[RAPL_SUM_ENERGY] = 0;

	if (detailedMode)
	{
		for (int die=0; die<NODE_SIZE_SOCKET; die++)
		{
			uint64_t dieEnergy = 0;
			for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
			{
				if ((iterator->first.back()-'0') != die)
					continue;
				std::string itemName = iterator->first.substr(0,iterator->first.length() - suffix.length() - 1);
				read_msr_by_idx(die * SOCKET_SIZE_CPU, counterDefines[itemName], &dieEnergy);
				record [itemName+suffix+std::to_string(die)] = (dieEnergy & MSR_PKG_ENERGY_STATUS_mask);
			}
		}
	}else
	{
		for (int die=0; die<NODE_SIZE_SOCKET; die++)
		{
			uint64_t dieEnergy = 0;

			for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
			{
				read_msr_by_idx(die * SOCKET_SIZE_CPU, counterDefines[iterator->first], &dieEnergy);
				record [iterator->first] += (dieEnergy & MSR_PKG_ENERGY_STATUS_mask);
			}
		}	
	}
}
#endif

std::string RAPL::getCountersLabel()
{
	return "RAPL";
}

std::string RAPL::getCounterLabel(std::string counter)
{
	std::map<std::string, std::string>::iterator it = counterLabel.find(counter);
	if (it != counterLabel.end())
		return it->second;
	else
		return counter;
}

std::string RAPL::getSummaryCounterName()
{
	return RAPL_SUM_ENERGY;
}
void RAPL::getSummaryValue(double time, unsigned long long int * counterSummary)
{
	*counterSummary += time * RAPL_BASELINE;
}

//	set list of counters to measure
void RAPL::initMap(std::map<std::string, unsigned long long int> & dict)
{
// fill this vector
	std::vector <std::string> names;
	names.push_back(RAPL_PCKG_ENERGY);
	names.push_back(RAPL_RAM_ENERGY);

// in detailed mode RAPL gives us values for each cpu 
// this is a reason for adding a suffix with number of a socket for each counter

//	suffix = "_";	//set suffix you want

// DO NOT EDIT THE REST OF THIS FUNCTION

	if (detailedMode)
	{
		for(int die = 0; die != NODE_SIZE_SOCKET; die++)
		{
			for(std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it)
			{
				dict[*it + suffix + std::to_string(die)] = 0;
			}
		}
	}else
	{
		for(std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it)
		{
			dict[*it] = 0;
			dict[*it] = 0;
		}
		
	}
}




