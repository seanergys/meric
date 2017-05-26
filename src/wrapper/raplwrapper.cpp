#ifdef HAVE_X86_ADAPT_H
#include "x86_adapt.h"

/* RAPL description
 *
 * Intel_RAPL_Pckg_Energy - all energy consumtions
 * Intel_RAPL_PP0_Energy  - CPU
 * Intel_RAPL_PP1_Energy  - may reflect to uncore devices
 * Intel_RAPL_RAM_Energy  - RAM
 *
 * Intel_RAPL_Pckg_Energy ~ Intel_RAPL_PP0_Energy + Intel_RAPL_PP1_Energy
 * Intel_RAPL_Pckg_Energy ~ Intel_RAPL_PP0_Energy + Intel_RAPL_RAM_Energy
 * 
 *
 * MSR_RAPL_POWER_UNIT Register
 * Power Units (bits 3:0):
 	Power related information (in Watts) is based on the multiplier, 1/2^PU; where PU is an unsigned integer represented by bits 3:0. Default value is 0011b, indicating power unit is in 1/8 Watts 
increment.
 * Energy Status Units (bits 12:8): 
 	Energy related information (in Joules) is based on the multiplier, 1/2^ESU; where ESU is an unsigned integer represented by bits 12:8. Default value is 10000b, indicating energy status unit is in 15.3 micro-Joules increment.
 * Time Units (bits 19:16): 
 Time related information (in Seconds) is based on the multiplier, 1/ 2^TU; where TU is an unsigned integer represented by bits 19:16. Default value is 1010b, indicating time unit is in 976 micro-seconds increment.

TIME    ENERGY     POWER
1010|000|01110|0000|0011
9876 543 21098 7654 3210
*/
const uint64_t RAPLpowerMask  = 0xF;
const uint64_t RAPLenergyMask = 0x1F00;
const uint64_t RAPLenergyShift= 8;
const uint64_t RAPLtimeMask   = 0xF0000;
const uint64_t RAPLtimeShift  = 16;

const int RAPL_BASELINE = 70; // 70 W
#endif

#include <math.h>
#include "raplwrapper.h"
#include "counters.h"
#include "../basis/utils.h"

using namespace meric;

std::string RAPL::suffix = "_";
int RAPL::numOfDevices = 1;
bool RAPL::detailedMode = false;
long long RAPL::COUNTERMAX = pow(2,32);

#ifdef HAVE_X86_ADAPT_H
std::map<std::string, long long> RAPL::counters;
std::map<std::string, double> RAPL::energyUnit;

// set counters units
void RAPL::init()
{
	numOfDevices = x86_adapt_get_nr_avaible_devices(X86_ADAPT_DIE);
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
	unit &= RAPLenergyMask;
	unit >>= RAPLenergyShift;
	if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
		std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
	
	energyUnit["Intel_RAPL_Pckg_Energy"] = 1.0/pow(2,unit);
	//the rest unit modifiers are set according to x86_adapt "documentation"
	//https://github.com/tud-zih-energy/x86_energy/blob/eac130973b2fbfa0b5a3e72ef9912af617584d97/rapl.c#L763
	energyUnit["Intel_RAPL_RAM_Energy"] = 1.0/pow(2,16);
	energyUnit["Intel_DRAM_ENERGY_STATUS_CH0"] = 1.0/pow(2,16);	//1.0/pow(2,18)
	energyUnit["Intel_DRAM_ENERGY_STATUS_CH1"] = energyUnit["Intel_DRAM_ENERGY_STATUS_CH0"];
	energyUnit["Intel_DRAM_ENERGY_STATUS_CH2"] = energyUnit["Intel_DRAM_ENERGY_STATUS_CH0"];
	energyUnit["Intel_DRAM_ENERGY_STATUS_CH3"] = energyUnit["Intel_DRAM_ENERGY_STATUS_CH0"];

}

//	read counters values
void RAPL::getValues(std::map<std::string, long long> & record) 
{
	for(std::map<std::string, long long>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
		record[iterator->first] = 0;
	
	if (detailedMode)
	{
		for (int die=0; die<numOfDevices; die++)
		{
			uint64_t dieEnergy = 0;
			const int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, die);
			if (fd < 0)
			{
				std::cerr << "X86ADAPT RAPL error while accessing file descriptor\n";
	//			exit(2);
			}
		
			for(std::map<std::string, long long>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
			{
	//TODO this version doesn't count with more than 10 sockets per node (should be enough for this century)
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
				record [itemName+suffix+std::to_string(die)] = dieEnergy*energyUnit[itemName];
			}
			if (x86_adapt_put_device(X86_ADAPT_DIE, die))
				std::cerr << "X86ADAPT RAPL error while closing file descriptor\n";
		}
	}else
	{
		for (int die=0; die<numOfDevices; die++)
		{
			uint64_t dieEnergy = 0;
			const int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, die);
			if (fd < 0)
			{
				std::cerr << "X86ADAPT RAPL error while accessing file descriptor\n";
		//			exit(2);
			}
			for(std::map<std::string, long long>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
			{
				const int itemId = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, iterator->first.c_str());
				x86_adapt_get_setting(fd, itemId, &dieEnergy);
				record [iterator->first] += dieEnergy*energyUnit[iterator->first];
			}
			if (x86_adapt_put_device(X86_ADAPT_DIE, die))
				std::cerr << "X86ADAPT RAPL error while closing file descriptor\n";
		}	
	}
}

// solve counter overflow and add baseline energy consumption
unsigned long long RAPL::getResultValue(long long startValue, long long stopValue, std::string counter, double runtime)
{
	unsigned long long value = RAPL_BASELINE * runtime;
	if (stopValue < startValue)
	{	
		if (!energyUnit.count(counter))
		{
			//missing unit modifier
			return 0;
		}
		else
		{
			MERIC_INFO << "RAPL OVERFLOW, MAX VALUE: " <<COUNTERMAX * energyUnit[counter] <<" " <<counter << std::endl;
			MERIC_INFO << "RAPL REPAIR " << startValue << " .. " << stopValue << " = " << COUNTERMAX * energyUnit[counter] - startValue + stopValue << std::endl;
			return value + COUNTERMAX * energyUnit[counter] - startValue + stopValue;
		}
	}
	else
		return value + stopValue - startValue;
}

#else
#endif

//	set list of counters to measure
void RAPL::initMap(std::map<std::string, long long> & dict)
{
// fill this vector
	std::vector <std::string> names;
	names.push_back(RAPL_COUNTERS::RAPL_PCKG_ENERGY);
	names.push_back(RAPL_COUNTERS::RAPL_RAM_ENERGY);

// in detailed mode RAPL gives us values for each cpu 
// this is a reason for adding a suffix with number of a socket for each counter

//	suffix = "_";	//set suffix you want

// DO NOT EDIT THE REST OF THIS FUNCTION

	if (detailedMode)
	{
		for(int die = 0; die != numOfDevices; die++)
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



