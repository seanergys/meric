#include <limits>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <omp.h>
#include "midlib.h"
#include "../wrapper/environmentwrapper.h"
#include "../wrapper/a64fxwrapper.h"
#include "../wrapper/raplwrapper.h"
#include "../wrapper/nvmlwrapper.h"
#include "../basis/utils.h"

#define POWER_BASELINE 0
std::vector<std::map<std::string, unsigned long long int>> recordEnergy;

template <typename Tmode>
void measureEnergy(unsigned long long int *cpuEnergy, unsigned long long int *dramEnergy, unsigned long long int *L2Energy)
{
    if (recordEnergy.empty())
    {
        printf("Initializing energy counters...\n");
        Tmode::init(true);
    }

	if (timeNow == 0.0)
	{
		struct timespec timestamp;
		clock_gettime(CLOCK_REALTIME, &timestamp);
		timeNow = meric::mutils::timeToDouble(timestamp);
	}
	std::map<std::string, unsigned long long int> energyNow;
	Tmode::getCounterValues(energyNow);
		
	recordEnergy.push_back(energyNow);
	recordTime.push_back(timeNow);
}

template void measureEnergy<meric::A64FX>(unsigned long long int *, unsigned long long int *, unsigned long long int *);
// template void measureEnergy<meric::RAPL>(unsigned long long int*,unsigned long long int*,unsigned long long int*);
// template void measureEnergy<meric::NVML>(unsigned long long int*,unsigned long long int*,unsigned long long int*);
// template void measureEnergy<meric::ROCM>(unsigned long long int*,unsigned long long int*,unsigned long long int*);
// template void measureEnergy<meric::HDEEM>(unsigned long long int*,unsigned long long int*,unsigned long long int*);

void midlib_init(bool is_detailed, MidlibEnergyDomains enabled_domains)
{
	meric::Environment::initHWSizes();

    if (MIDLIB_ENERGY_NO_SELECTION(enabled_domains))
    {
        
    }
    
    if (MIDLIB_ENERGY_HAS_DOMAIN(enabled_domains, MidlibEnergyDomains::A64FX))
    {

    }
    if(MIDLIB_ENERGY_HAS_DOMAIN(enabled_domains, MidlibEnergyDomains::RAPL))
    {

    }
    if(MIDLIB_ENERGY_HAS_DOMAIN(enabled_domains, MidlibEnergyDomains::NVML))
    {
        
    }
    if(MIDLIB_ENERGY_HAS_DOMAIN(enabled_domains, MidlibEnergyDomains::ROCM))
    {
        
    }
    if(MIDLIB_ENERGY_HAS_DOMAIN(enabled_domains, MidlibEnergyDomains::HDEEM))
    {
        
    }
    

}

void midlib_read_energy_measurements()
{
}

void midlib_read_energy_a64fx(unsigned long long int *cpuEnergy, unsigned long long int *dramEnergy, unsigned long long int *L2Energy)
{
    measureEnergy<meric::A64FX>(cpuEnergy, dramEnergy, L2Energy);
}
