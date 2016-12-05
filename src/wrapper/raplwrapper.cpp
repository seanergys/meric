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
const uint64_t RAPLenergyMask = 0xF00;
const uint64_t RAPLenergyShift= 8;
const uint64_t RAPLtimeMask   = 0xF0000;
const uint64_t RAPLtimeShift  = 16;
#endif

#include <math.h>
#include "raplwrapper.h"
#include "counters.h"

using namespace meric;


#ifdef HAVE_X86_ADAPT_H
std::map<std::string, long> RAPL::counters;
double RAPL::energyUnit = 1.0;

void RAPL::init()
{
	initMap(counters);

	uint64_t unit;
	const int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0); //ignoring fact that different nodes may have different units
	if (fd < 0)
	{
		std::cerr << "X86ADAPT RAPL INIT error while accessing file descriptor\n";
		std::cerr << "WARNING: RAPL values will be without any unit\n";
		return;
//		exit(2);
	}
	const int itemId = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_Power_Unit");
	x86_adapt_get_setting(fd, itemId, &unit);
	unit &= RAPLenergyMask;
	unit >>= RAPLenergyShift;
	energyUnit = 1.0/pow(2,unit);
	if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
		std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
}

void RAPL::getValues(std::map<std::string, long> & record) 
{
	for(std::map<std::string, long>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
		record[iterator->first] = 0;

	uint64_t energy = 0;
	const int max = x86_adapt_get_nr_avaible_devices(X86_ADAPT_DIE);
	for (int die=0; die<max; die++)
	{
		uint64_t dieEnergy = 0;
		const int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, die);
		if (fd < 0)
		{
			std::cerr << "X86ADAPT RAPL error while accessing file descriptor\n";
//			exit(2);
		}
		
		for(std::map<std::string, long>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
		{
			const int itemId = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, iterator->first.c_str());
			x86_adapt_get_setting(fd, itemId, &dieEnergy);
			energy += dieEnergy;

			record [iterator->first] += energy*energyUnit;
		}
		if (x86_adapt_put_device(X86_ADAPT_DIE, die))
			std::cerr << "X86ADAPT RAPL error while closing file descriptor\n";
	}
	
}

#else
#endif

void RAPL::initMap(std::map<std::string, long> & dict)
{
	dict[RAPL_COUNTERS::RAPL_PCKG_ENERGY] = 0;
	dict[RAPL_COUNTERS::RAPL_PP0_ENERGY]  = 0;
	dict[RAPL_COUNTERS::RAPL_RAM_ENERGY]  = 0;
}



