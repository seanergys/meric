#include "raplwrapper.h"
#include <cstdio>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "environmentwrapper.h"
#include "counters.h"
#include "../basis/sysfsutils.h"

#ifdef HAVE_HSR_SAFE
	namespace MSR_RAPL_COUNTERS_NS = MSR_RAPL_COUNTERS;
#elif HAVE_X86_ADAPT_H
	namespace MSR_RAPL_COUNTERS_NS = X86ADAPT_RAPL_COUNTERS;
#elif defined HAVE_GEOPM_H
	namespace MSR_RAPL_COUNTERS_NS = GEOPM_RAPL_COUNTERS;
#else
	namespace MSR_RAPL_COUNTERS_NS = MSR_RAPL_COUNTERS;
#endif


using namespace meric;

std::string RAPL::PCKG_POWER_LIMIT = "";
std::string RAPL::RAM_POWER_LIMIT = "";
std::string RAPL::PLATFORM_POWER_LIMIT = "";
std::string RAPL::POWER_LIMIT_TIMEWINDOW = "";

void RAPL::init(bool detailed) {
    getImpl()->init(detailed);
}

void RAPL::close() {
    getImpl()->close();
}

void RAPL::getCounterValues(CounterRecord_t &record) {
    getImpl()->getCounterValues(record);
}

double RAPL::getResultValue(RawCounterValue_t startValue, RawCounterValue_t stopValue, 
    double runtime, int &cumulative, std::string counter, std::string regionName) {
    return getImpl()->getResultValue(startValue, stopValue, runtime, cumulative, counter);
}

std::string RAPL::summaryCounter() {
    return getImpl()->summaryCounter();
}

std::string RAPL::getSummaryCounterName() {
    return getImpl()->getSummaryCounterName();
}

std::string RAPL::getSourceLabel() {
    return getImpl()->getSourceLabel();
}

std::string RAPL::getCounterLabel(const std::string counter) {
    return getImpl()->getCounterLabel(counter);
}

void RAPL::getRatios(CounterRecord_t &start, CounterRecord_t &stop,
    double runtime, std::map<std::string, double> &ratios) {
    getImpl()->getRatios(start, stop, runtime, ratios);
}

void RAPL::initCountersUnits() {
    getImpl()->initCountersUnits();
}

double RAPL::getUnit(std::string counter) {
    return getImpl()->getUnit(counter);
}

class RAPL_NONE : public RAPL::Base {
public:
    virtual void init(bool detailed) {}
    virtual void close() {}

    virtual void getCounterValues(CounterRecord_t &record) {}
    virtual double getResultValue(RawCounterValue_t startValue, RawCounterValue_t stopValue, 
        double runtime, int &cumulative, std::string counter, std::string regionName = "") { return 0.0; }

    virtual std::string summaryCounter() { return ""; }
    virtual std::string getSummaryCounterName() { return ""; }
    virtual std::string getSourceLabel() { return ""; }
    virtual std::string getCounterLabel(const std::string counter) { return ""; }

    virtual void getRatios(CounterRecord_t &start, CounterRecord_t &stop,
        double runtime, std::map<std::string, double> &ratios) {}

    virtual void initCountersUnits() {}
    virtual double getUnit(std::string counter) { return 0.0; }

    static bool isAvailable() { return true; }
};

#if defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_GEOPM_H

class RAPL_MSR : public RAPL::Base {
public:
    RAPL_MSR();

    virtual void init(bool detailed);
    virtual void close();

    virtual void getCounterValues(CounterRecord_t &record);
    virtual double getResultValue(RawCounterValue_t startValue, RawCounterValue_t stopValue, 
        double runtime, int &cumulative, std::string counter, std::string regionName = "");

    virtual std::string summaryCounter();
    virtual std::string getSummaryCounterName();
    virtual std::string getSourceLabel();
    virtual std::string getCounterLabel(const std::string counter);

    virtual void getRatios(CounterRecord_t &start, CounterRecord_t &stop,
        double runtime, std::map<std::string, double> &ratios) {}

    virtual void initCountersUnits();
    virtual double getUnit(std::string counter);

    static bool isAvailable();

protected:
    void initMap(CounterRecord_t &record);

    std::string suffix;
    bool detailedMode;
    RawCounterValue_t COUNTERMAX;
    CounterRecord_t counters;
    std::map<std::string, double> units;
};

RAPL_MSR::RAPL_MSR()
    : suffix("_")
    , detailedMode(false)
    , COUNTERMAX(uint64_t(1) << 32)
{
}

void RAPL_MSR::init(bool detailed)
{
    detailedMode = detailed;
    initMap(counters);
}

void RAPL_MSR::close()
{
}

void RAPL_MSR::getCounterValues(CounterRecord_t &record)
{
    using namespace MSR_RAPL_COUNTERS_NS;

#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_GEOPM_H
    for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
    {
        record[iterator->first] = 0;
    }

    if (detailedMode)
    {
        for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
        {
            uint64_t domainEnergy = 0;
            std::string itemName = iterator->first.substr(0, iterator->first.find_last_of('_'));
            int die = std::stoi(iterator->first.substr(itemName.length()+1));

#ifdef _AMD_
            if (itemName == RAPL_CORE_ENERGY)
            {
            #ifdef HAVE_MSR_SAFE
                read_msr(Environment::mapThreadsToCore[die][0], counterDefines[itemName], &domainEnergy);
            #elif defined HAVE_GEOPM_H
                domainEnergy = geopm_signal_to_field(pio.read_signal(itemName, pio.signal_domain_type(itemName), Environment::mapThreadsToCore[die][0]));
            #endif
            }
            else
#endif
            {
                #if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H
                    read_msr(Environment::mapCoresToSocket[die][0], counterDefines[itemName], &domainEnergy);
                #elif defined HAVE_GEOPM_H
                    domainEnergy = geopm_signal_to_field(pio.read_signal(itemName, pio.signal_domain_type(itemName), Environment::mapCoresToSocket[die][0]));
                #endif
            }
            record [iterator->first] = (domainEnergy & MSR_ANYDOMAIN_ENERGY_STATUS_mask);
        }
    }
    else
    {
        for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
        {
#ifdef _AMD_
            if (iterator->first == RAPL_CORE_ENERGY)
            {
                for (int die=0; die<Environment::SOCKETS_PER_NODE*Environment::CORES_PER_SOCKET; die++)
                {
                    uint64_t domainEnergy = 0;
                    #ifdef HAVE_MSR_SAFE
                        read_msr(Environment::mapThreadsToCore[die][0], counterDefines[iterator->first], &domainEnergy);
                    #elif defined HAVE_GEOPM_H
                        domainEnergy = geopm_signal_to_field(pio.read_signal(iterator->first, pio.signal_domain_type(iterator->first), Environment::mapThreadsToCore[die][0]));
                    #endif
                    record [iterator->first] += (domainEnergy & MSR_ANYDOMAIN_ENERGY_STATUS_mask);
                }
            }
            else
#endif
            {
                for (int die=0; die<Environment::SOCKETS_PER_NODE; die++)
                {
                    uint64_t domainEnergy = 0;
                    #if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H
                        read_msr(Environment::mapCoresToSocket[die][0], counterDefines[iterator->first], &domainEnergy);
                    #elif defined HAVE_GEOPM_H
                        domainEnergy = geopm_signal_to_field(pio.read_signal(iterator->first, pio.signal_domain_type(iterator->first), Environment::mapCoresToSocket[die][0]));
                    #endif
                    record [iterator->first] += (domainEnergy & MSR_ANYDOMAIN_ENERGY_STATUS_mask);
                }
            }
        }	
    }
#elif defined _INTEL_ && defined HAVE_X86_ADAPT_H
    for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
    {
        record[iterator->first] = 0;
    }

    if (detailedMode)
    {
        for (int die=0; die<Environment::SOCKETS_PER_NODE; die++)
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
                if (std::stoi(iterator->first.substr(iterator->first.find_last_of('_')+1)) != die)
                {
                    continue;
                }
                std::string itemName = iterator->first.substr(0, iterator->first.find_last_of('_'));
                const int itemId = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, itemName.c_str());
                if (itemId < 0) //== -ENXIO
                {
                    std::cerr << "X86ADAPT RAPL wrong counter name - " << itemName <<std::endl;
                    continue;
                }
                if (!units.count(itemName))
                {
                    std::cerr << "X86ADAPT RAPL missing unit modifier for counter - " << itemName <<" Add modifier in RAPL::init()"<<std::endl;
                    continue;					
                }
                x86_adapt_get_setting(fd, itemId, &dieEnergy);
                record [iterator->first] = (dieEnergy & MSR_ANYDOMAIN_ENERGY_STATUS_mask);
            }
            if (x86_adapt_put_device(X86_ADAPT_DIE, die))
            {
                std::cerr << "X86ADAPT RAPL error while closing file descriptor\n";
            }
        }
    }
    else
    {
        for (int die=0; die<Environment::SOCKETS_PER_NODE; die++)
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
                record [iterator->first] += (dieEnergy & MSR_ANYDOMAIN_ENERGY_STATUS_mask);
            }
            if (x86_adapt_put_device(X86_ADAPT_DIE, die))
                std::cerr << "X86ADAPT RAPL error while closing file descriptor\n";
        }	
    }
#endif
}

double RAPL_MSR::getResultValue(RawCounterValue_t startValue, RawCounterValue_t stopValue, 
    double runtime, int &cumulative, std::string counter, std::string regionName)
{
    using namespace MSR_RAPL_COUNTERS_NS;

	std::string counterName;
	if (detailedMode)
	{
		counterName = counter.substr(0, counter.find_last_of('_'));
	}
	else
	{
		counterName = counter;
	}

#ifdef _INTEL_
	cumulative = 1;
#else //_AMD_
	if (counterName == RAPL_PCKG_ENERGY)
	{
		cumulative = 1;
	}
	else
	{
		cumulative = 0;
	}
#endif
	
	if (stopValue < startValue)
	{	
		if (units.count(counterName))
		{
			MERIC_INFO << "RAPL OVERFLOW, MAX VALUE: " <<COUNTERMAX << "*"<< units[counterName] <<" " <<counterName << std::endl;
			MERIC_INFO << "RAPL REPAIR " << startValue << " .. " << stopValue << " = " << ((COUNTERMAX - startValue) + stopValue) * units[counterName] << std::endl;
			return ((COUNTERMAX - startValue) + stopValue) * units[counterName];
		} //else: unit modifier is missing
		return 0.0;
	}
	else
	{
		return (stopValue - startValue) * units[counterName];
	}
}

std::string RAPL_MSR::summaryCounter()
{
    using namespace MSR_RAPL_COUNTERS_NS;

    return RAPL_SUM_ENERGY;
}

std::string RAPL_MSR::getSummaryCounterName()
{
    using namespace MSR_RAPL_COUNTERS_NS;

#ifdef _INTEL_
		return RAPL_SUM_ENERGY;
#else // _AMD_
		if (detailedMode)
		{
  			return RAPL_SUM_ENERGY;
  		}
		else
		{
			return "";
		}
#endif
}

std::string RAPL_MSR::getSourceLabel()
{
    return "RAPL";
}

std::string RAPL_MSR::getCounterLabel(const std::string counter)
{
    using namespace MSR_RAPL_COUNTERS_NS;

	std::map<const std::string, std::string>::iterator it = counterLabel.find(counter);
	if (it != counterLabel.end())
	{
		return it->second;
	}
	else
	{
		return counter;
	}
}

void RAPL_MSR::initCountersUnits()
{
    using namespace MSR_RAPL_COUNTERS_NS;

    RAPL::PCKG_POWER_LIMIT       = RAPL_PCKG_POWER_LIMIT;
    RAPL::RAM_POWER_LIMIT        = RAPL_RAM_POWER_LIMIT;
    RAPL::PLATFORM_POWER_LIMIT   = RAPL_PLATFORM_POWER_LIMIT;
    RAPL::POWER_LIMIT_TIMEWINDOW = RAPL_POWER_LIMIT_TIMEWINDOW;

	if (units.size())
	{
		return; //already initialized
	}
	uint64_t MSR_RAPL_POWER_UNIT_bits = 0x0;
#if (defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H) && defined MSR_RAPL_POWER_UNIT
	read_msr(0, MSR_RAPL_POWER_UNIT, &MSR_RAPL_POWER_UNIT_bits);
#elif defined HAVE_X86_ADAPT_H
	const int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
	if (fd < 0)
	{
		std::cerr << "X86ADAPT RAPL INIT error while accessing file descriptor\n";
		return;
	}
	const int itemId = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_Power_Unit");
	x86_adapt_get_setting(fd, itemId, &MSR_RAPL_POWER_UNIT_bits);
	if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
	{
		std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
	}
#elif defined HAVE_GEOPM_H
	MSR_RAPL_POWER_UNIT_bits = geopm_signal_to_field(pio.read_signal("MSR::RAPL_POWER_UNIT#", pio.signal_domain_type("MSR::RAPL_POWER_UNIT#"), 0));
#endif
	if (MSR_RAPL_POWER_UNIT_bits != 0)
	{
		//power
		units[RAPL_PCKG_POWER_LIMIT] = 1.0/pow(2,(MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_power_mask));
		units[RAPL_RAM_POWER_LIMIT] = units[RAPL_PCKG_POWER_LIMIT];
#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H
		units[RAPL_PLATFORM_POWER_LIMIT] = units[RAPL_PCKG_POWER_LIMIT];
#endif
		//energy
#ifdef MSR_PKG_ENERGY_STATUS
		units[RAPL_PCKG_ENERGY] = 1.0/pow(2,((MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_energy_mask) >> MSR_RAPL_POWER_UNIT_energy_shift));
#endif
		//DRAM ENERGY unit is fixed to 15.3uJ
		//https://www.intel.com/content/dam/www/public/us/en/documents/datasheets/xeon-e5-v3-datasheet-vol-2.pdf
#ifdef MSR_DRAM_ENERGY_STATUS
		units[RAPL_RAM_ENERGY] = 1.0/pow(2,16);
#endif
#ifdef MSR_PLATFORM_ENERGY_COUNTER
		units[RAPL_PLATFORM_ENERGY] = units[RAPL_PCKG_ENERGY];
#endif
#ifdef MSR_CORE_ENERGY_STATUS
		units[RAPL_CORE_ENERGY] = units[RAPL_PCKG_ENERGY];
#endif

		//time
		units[RAPL_POWER_LIMIT_TIMEWINDOW] = 1.0/pow(2,((MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_time_mask) >> MSR_RAPL_POWER_UNIT_time_shift));
		units[MSR_COUNTERS::RAPL_PKG_ACTIVE] = units[RAPL_POWER_LIMIT_TIMEWINDOW];
	}
}

double RAPL_MSR::getUnit(std::string counter)
{
	if (units.count(counter))
	{
		return units[counter];
	}
	else
	{
		return -1.0;
	}
}

void RAPL_MSR::initMap(CounterRecord_t &dict)
{
    using namespace MSR_RAPL_COUNTERS_NS;

    // fill this vector
    std::vector <std::string> names;

#ifdef _INTEL_
	names.push_back(RAPL_PCKG_ENERGY);
	names.push_back(RAPL_RAM_ENERGY);
#elif defined _AMD_
	names.push_back(RAPL_PCKG_ENERGY);
	if (detailedMode)
	{
		names.push_back(RAPL_CORE_ENERGY);
	}
	else
	{
		counterLabel[RAPL_PCKG_ENERGY] = RAPL_SUM_ENERGY;
	}
#endif

    // in detailed mode RAPL gives us values for each cpu 
    // this is a reason for adding a suffix with number of a socket for each counter
    if (detailedMode)
    {
    #ifdef _AMD_
        std::vector<std::string>::iterator it = find(names.begin(), names.end(), RAPL_CORE_ENERGY);
        if (it != names.end())
        {
            const int suffixLength = log10(Environment::CORES_PER_SOCKET*Environment::SOCKETS_PER_NODE)+1;
            for(int core = 0; core != Environment::CORES_PER_SOCKET*Environment::SOCKETS_PER_NODE; core++)
            {
                std::ostringstream os;
                os << std::setfill('0') << std::setw(suffixLength) << core;
                const std::string counterName = RAPL_CORE_ENERGY + suffix + os.str();
                dict[counterName] = 0;
                
                counterLabel[counterName] = counterLabel[RAPL_CORE_ENERGY].substr(0,counterLabel[RAPL_CORE_ENERGY].find(' '))+suffix + os.str() + counterLabel[RAPL_CORE_ENERGY].substr(counterLabel[RAPL_CORE_ENERGY].find(' '));

    #ifdef HAVE_MSR_SAFE
                counterDefines[counterName] = counterDefines[RAPL_CORE_ENERGY];
    #endif
            }
            names.erase(it);
        }
    #endif

        const int suffixLength = log10(Environment::SOCKETS_PER_NODE)+1;
        for(int die = 0; die != Environment::SOCKETS_PER_NODE; die++)
        {
            for(std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it)
            {
                std::ostringstream os;
                os << std::setfill('0') << std::setw(suffixLength) << die;
                const std::string counterName = *it + suffix + os.str();
                dict[counterName] = 0;
                
                counterLabel[counterName] = counterLabel[*it].substr(0,counterLabel[*it].find(' '))+suffix + os.str() + counterLabel[*it].substr(counterLabel[*it].find(' '));

    #if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H
                counterDefines[counterName] = counterDefines[*it];
    #endif
            }
        }
    }
    else
    {
        for(std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it)
        {
            dict[*it] = 0;
        }
    }
}

bool RAPL_MSR::isAvailable()
{
    using namespace MSR_RAPL_COUNTERS_NS;

    return check_whitelist_msr({});
}

#endif

class RAPL_SYSFS : public RAPL::Base {
public:
    virtual void init(bool detailed);
    virtual void close();

    virtual void getCounterValues(CounterRecord_t &record);
    virtual double getResultValue(RawCounterValue_t startValue, RawCounterValue_t stopValue, 
        double runtime, int &cumulative, std::string counter, std::string regionName = "");

    virtual std::string summaryCounter();
    virtual std::string getSummaryCounterName();
    virtual std::string getSourceLabel();
    virtual std::string getCounterLabel(const std::string counter);

    virtual void getRatios(CounterRecord_t &start, CounterRecord_t &stop,
        double runtime, std::map<std::string, double> &ratios) {}

    virtual void initCountersUnits() {}
    virtual double getUnit(std::string counter);

    static bool isAvailable();

protected:
    struct CounterDesc_t {
        std::string label;
        uint64_t rangeMax;
        double unit;
        std::string path;
        std::ifstream fs;
        bool isCumulative;
    };

    std::string suffix;
    int numOfDevices;
    bool detailedMode;
    std::map<std::string, CounterDesc_t>     counterDesc;
    std::map<std::string, RawCounterValue_t> counters;
    std::map<std::string, double>            units;
};

void RAPL_SYSFS::init(bool detailed)
{
    using namespace SYSFS_RAPL_COUNTERS;

    detailedMode = detailed;

    std::vector<std::string> names;

#ifdef _INTEL_
	names.push_back(RAPL_PCKG_ENERGY);
	names.push_back(RAPL_RAM_ENERGY);
#elif defined _AMD_
	names.push_back(RAPL_PCKG_ENERGY);
	names.push_back(RAPL_PCKG_CORES_ENERGY);
#endif

    const std::string sysfsRAPLRoot("/sys/class/powercap/intel-rapl");
    for(int die = 0; die < Environment::SOCKETS_PER_NODE; ++die)
    {
        const std::string dieRootPath = sysfsRAPLRoot + ":" + std::to_string(die);
        const std::vector<const char *> dieDomainNames{
            "",
            ":0"
        };

        for(const char *pszDomainName: dieDomainNames)
        {
            std::string sysfsName;
            uint64_t maxCounterRange = 0;

            bool state = sysfsutils::ReadValue(dieRootPath + pszDomainName + "/name", sysfsName);
            state = state && sysfsutils::ReadValue(dieRootPath + pszDomainName + "/max_energy_range_uj", maxCounterRange);

            if(!state)
            {
                continue;
            }

            std::string counterBaseName;

            for(const std::string &name: names)
            {
                if(sysfsName.rfind(name, 0) == 0)
                {
                    counterBaseName = name;
                }
            }

            if(counterBaseName.empty())
            {
                continue;
            }

            const std::string fullName  = sysfsutils::MakeCounterName(counterBaseName, counterLabel, {{ die, 1 }});
            const std::string fullLabel = sysfsutils::MakeCounterName(counterBaseName, counterLabel, {{ die, 1 }}, true);

            units[fullName]    = 1.0e-6;
            counters[fullName] = 0;
            counterDesc[fullName] = CounterDesc_t{
                fullLabel,
                maxCounterRange,
                1.0e-6,
                dieRootPath + pszDomainName + "/energy_uj",
                std::ifstream(dieRootPath + pszDomainName + "/energy_uj"),
#ifdef _INTEL_
                true
#elif defined _AMD_
                counterBaseName == RAPL_PCKG_ENERGY
#else
                false
#endif
            };
        }
    }
}

void RAPL_SYSFS::close()
{
    for(auto it = counterDesc.begin(); it != counterDesc.end(); ++it)
        it->second.fs.close();

    counterDesc.clear();
}

void RAPL_SYSFS::getCounterValues(CounterRecord_t &record)
{
	for(const auto &desc: counterDesc)
	{
		record[desc.first] = 0;
	}

	for(auto &desc: counterDesc)
	{
		uint64_t value = 0;
		sysfsutils::ReadValue(desc.second.fs, value);
		record[desc.first] = value;
	}
}

double RAPL_SYSFS::getResultValue(RawCounterValue_t startValue, RawCounterValue_t stopValue, 
    double runtime, int &cumulative, std::string counter, std::string regionName)
{
	auto it = counterDesc.find(counter);
	if(it == counterDesc.end())
	{
		return 0.0;
	}

	cumulative = it->second.isCumulative ? 1 : 0;
	if(stopValue < startValue)
	{
		return ((it->second.rangeMax - startValue) + stopValue) * it->second.unit;
	}

	return (stopValue - startValue) * it->second.unit;
}

std::string RAPL_SYSFS::summaryCounter()
{
    using namespace SYSFS_RAPL_COUNTERS;

    return RAPL_SUM_ENERGY;
}

std::string RAPL_SYSFS::getSummaryCounterName()
{
    using namespace SYSFS_RAPL_COUNTERS;

	auto it = counterLabel.find(summaryCounter());
	if(it != counterLabel.end())
	{
		return it->second.first + " " + it->second.second;
	}
	return summaryCounter();
}

std::string RAPL_SYSFS::getSourceLabel()
{
    return "RAPL";
}

std::string RAPL_SYSFS::getCounterLabel(const std::string counter)
{
    using namespace SYSFS_RAPL_COUNTERS;

    auto it = counterDesc.find(counter);
	if(it != counterDesc.end())
	{
		return it->second.label;
	}
	return counter;
}

double RAPL_SYSFS::getUnit(std::string counter)
{
    if (units.count(counter))
	{
		return units[counter];
	}

	return -1.0;
}

bool RAPL_SYSFS::isAvailable()
{
    if(access("/sys/class/powercap/intel-rapl:0/name", R_OK) != 0)
        return false;

    if(access("/sys/class/powercap/intel-rapl:0/energy_uj", R_OK) != 0)
        return false;

    return true;
}

class RAPL_PERF_EVENT : public RAPL::Base {
public:
    virtual void init(bool detailed);
    virtual void close();

    virtual void getCounterValues(CounterRecord_t &record);
    virtual double getResultValue(RawCounterValue_t startValue, RawCounterValue_t stopValue, 
        double runtime, int &cumulative, std::string counter, std::string regionName = "");

    virtual std::string summaryCounter();
    virtual std::string getSummaryCounterName();
    virtual std::string getSourceLabel();
    virtual std::string getCounterLabel(const std::string counter);

    virtual void getRatios(CounterRecord_t &start, CounterRecord_t &stop,
        double runtime, std::map<std::string, double> &ratios) {}

    virtual void initCountersUnits() {}
    virtual double getUnit(std::string counter);

    static bool isAvailable();

protected:
    template<size_t S = 8>
    struct Values_t {
        struct value {
            uint64_t value;
            uint64_t id;
        };
        
        uint64_t count_members;
        uint64_t time_enabled;
        uint64_t time_running;
        
        std::array<value, S> values;
    };

    struct CounterDesc_t {
        CounterDesc_t()
            : fd(-1)
        {}

        CounterDesc_t(const CounterDesc_t &other) = delete;

        CounterDesc_t(CounterDesc_t &&other)
            : fd(other.fd)
            , unit(other.unit)
            , label(std::move(other.label))
            , isCumulative(other.isCumulative)
        {
            other.fd = -1;
        }

        ~CounterDesc_t() {
            close();
        }

        bool open(uint32_t type, uint64_t config, int cpu) {
            perf_event_attr event_attrib;
            std::memset(&event_attrib, 0, sizeof(perf_event_attr));

            event_attrib.type        = type;
            event_attrib.size        = sizeof(perf_event_attr);
            event_attrib.config      = config;
            event_attrib.sample_type = PERF_SAMPLE_IDENTIFIER;
            event_attrib.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING | 
                PERF_FORMAT_GROUP | PERF_FORMAT_ID;
            event_attrib.disabled    = 1;
            event_attrib.inherit     = 1;

            fd = syscall(__NR_perf_event_open,
                &event_attrib, pid_t(-1), cpu, int(-1), 0);
            
            return (fd >= 0);
        }

        void close() {
            if(fd < 0)
                return;

            disable();
            ::close(fd);
            fd = -1;
        }

        int reset()   { if(fd < 0) return -1; return ::ioctl(fd, PERF_EVENT_IOC_RESET, 0); }
        int enable()  { if(fd < 0) return -1; return ::ioctl(fd, PERF_EVENT_IOC_ENABLE, 0); }
        int disable() { if(fd < 0) return -1; return ::ioctl(fd, PERF_EVENT_IOC_DISABLE, 0); }

        bool read(Values_t<> &values) {
            if(fd < 0)
                return false;

            return ::read(fd, &values, sizeof(Values_t<>)) >= sizeof(Values_t<1>);
        }

        int fd;
        double unit;
        std::string label;
        uint64_t rangeMax;
        bool isCumulative;
    };

    std::map<std::string, CounterDesc_t> counterDesc;
};

void RAPL_PERF_EVENT::init(bool detailed)
{
    using namespace PERF_RAPL_COUNTERS;

    const int socketSuffixLength = std::log10(Environment::SOCKETS_PER_NODE)+1;
    const int coreSuffixLength   = std::log10(Environment::SOCKETS_PER_NODE*Environment::CORES_PER_SOCKET)+1;

    auto OpenCounter = [&](const CounterDef_t &def, int cpu, int socket, int core) {
        std::vector<std::pair<int, int> > numbers;
        if(socket >= 0)
            numbers.push_back({ socket, socketSuffixLength });

        if(core >= 0)
            numbers.push_back({ core, coreSuffixLength });

        const std::string fullName = sysfsutils::MakeCounterName(def.label, 
            def.label, def.unit, numbers);

        auto res = counterDesc.insert(std::make_pair(fullName, CounterDesc_t()));
        if(!res.second)
            return true;

        if(!res.first->second.open(def.type, def.config, cpu))
        {
            std::cerr << "PERF_EVENT: Failed to open perf event" << std::endl;
            return false;
        }

        res.first->second.unit         = 1.0 / double(uint64_t(1) << 32);
        res.first->second.label        = def.label;
        res.first->second.rangeMax     = std::numeric_limits<uint64_t>::max();
        res.first->second.isCumulative = (def.flags & CounterDef_t::FLAG_ACCUMULATIVE_BIT) != 0;

        return true;
    };

    auto FindCounterByName = [&](CounterDef_t &def) {
        const std::string basePath = "/sys/bus/event_source/devices/";

        if(def.name.size() < 2)
            return true;

        if(!sysfsutils::ReadValue(basePath + "/" + def.name[0] + "/type", def.type))
            return false;

        std::string event;
        if(!sysfsutils::ReadValue(basePath + "/" + def.name[0] + "/events/" + def.name[1], event))
            return false;

        if(sscanf(event.c_str(), "event=%lx", &def.config) < 1)
            return false;

        return true;
    };

    for(const auto &counterDef: counterDefs)
    {
        auto def = counterDef.second;

        if(!FindCounterByName(def))
            continue;

        switch(def.domain)
        {
        case CounterDef_t::DOMAIN_PER_SOCKET:
            for(int i = 0; i < Environment::SOCKETS_PER_NODE; ++i)
            {
                OpenCounter(def, i * Environment::CORES_PER_SOCKET, i, -1);
            }
            break;
        case CounterDef_t::DOMAIN_PER_CORE:
            for(int i = 0; i < Environment::SOCKETS_PER_NODE*Environment::CORES_PER_SOCKET; ++i)
            {
                OpenCounter(def, i, 
                    i / Environment::CORES_PER_SOCKET, i % Environment::CORES_PER_SOCKET);
            }
            break;
        default:
            break;
        }
    }

    for(auto it = counterDesc.begin(); it != counterDesc.end(); ++it)
    {
        it->second.reset();
        it->second.enable();
    }
}

void RAPL_PERF_EVENT::close()
{
    counterDesc.clear();
}

void RAPL_PERF_EVENT::getCounterValues(CounterRecord_t &record)
{
    for(auto it = counterDesc.begin(); it != counterDesc.end(); ++it)
    {
        Values_t<> values;
        if(!it->second.read(values))
        {
            std::cerr << "PERF_EVENT: Failed to read perf event" << std::endl;
            record[it->first] = 0;
            continue;
        }

        record[it->first] = values.values[0].value;
    }
}

double RAPL_PERF_EVENT::getResultValue(RawCounterValue_t startValue, RawCounterValue_t stopValue, 
    double runtime, int &cumulative, std::string counter, std::string regionName)
{
    auto it = counterDesc.find(counter);
    if(it == counterDesc.end())
    {
        return 0.0;
    }

    cumulative = it->second.isCumulative ? 1 : 0;
    if(stopValue < startValue)
    {
        return ((it->second.rangeMax - startValue) + stopValue) * it->second.unit;
    }

    return (stopValue - startValue) * it->second.unit;
}

std::string RAPL_PERF_EVENT::summaryCounter()
{
    using namespace PERF_RAPL_COUNTERS;

    return RAPL_SUM_ENERGY;
}

std::string RAPL_PERF_EVENT::getSummaryCounterName()
{
    using namespace PERF_RAPL_COUNTERS;

    auto it = counterDefs.find(summaryCounter());
    if(it != counterDefs.end())
    {
        return it->second.label + " " + it->second.unit;
    }

    return summaryCounter();
}

std::string RAPL_PERF_EVENT::getSourceLabel()
{
    return "RAPL";
}

std::string RAPL_PERF_EVENT::getCounterLabel(const std::string counter)
{
    using namespace PERF_RAPL_COUNTERS;

    auto it = counterDefs.find(counter);
    if(it != counterDefs.end())
    {
        return it->second.label;
    }

    return counter;
}

double RAPL_PERF_EVENT::getUnit(std::string counter)
{
    auto it = counterDesc.find(counter);
    if(it != counterDesc.end())
    {
        return it->second.unit;
    }

    return -1.0;
}

bool RAPL_PERF_EVENT::isAvailable()
{
    int value = 2;
    if(!sysfsutils::ReadValue("/proc/sys/kernel/perf_event_paranoid", value))
        return false;

    return (value <= 1);
}

RAPL::Base *RAPL::getImpl() {
    auto InitImpl = []() {
#if defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_GEOPM_H
        if(RAPL_MSR::isAvailable())
            return std::unique_ptr<Base>(new RAPL_MSR());
#endif

        if(RAPL_PERF_EVENT::isAvailable())
            return std::unique_ptr<Base>(new RAPL_PERF_EVENT());

        if(RAPL_SYSFS::isAvailable())
            return std::unique_ptr<Base>(new RAPL_SYSFS());
        
        return std::unique_ptr<Base>(new RAPL_NONE());
    };

    static std::unique_ptr<Base> pImpl = InitImpl();

    return pImpl.get();
}