#include "raplwrapper.h"
#include "environmentwrapper.h"
#include <fstream>
#include <string>

using namespace meric;

std::string RAPL::suffix = "_";
bool RAPL::detailedMode = false;
unsigned long long int RAPL::COUNTERMAX = pow(2,32);
std::map<std::string, double> RAPL::units;
std::map<std::string, RAPL::SysfsRAPLCounterDesc_t> RAPL::s_counterDesc;
std::map<std::string, unsigned long long int> RAPL::counters;

#if defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_GEOPM_H

/**
 * returns result value of the counter (std::string) based on its startValue and
 * stopValue and time between these two measurements
 * handles counter overflow
 **/
double RAPL::getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, int & cumulative, std::string counter, std::string regionName)
{
	std::string counterName;
	if (detailedMode)
		counterName = counter.substr(0, counter.find_last_of('_'));
	else
		counterName = counter;

#ifdef _INTEL_
	cumulative = 1;
#else //_AMD_
	if (counterName == RAPL_PCKG_ENERGY)
		cumulative = 1;
	else
		cumulative = 0;
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
		return (stopValue - startValue) * units[counterName];
}

/**
 * initialization of the RAPL class
 * initCountersUnits() is already called from environmentwrapper
 **/
void RAPL::init(bool detailed)
{
	detailedMode = detailed;
	initMap(counters);
	//initCountersUnits() is already called from environmentwrapper
}

/**
 * stores the RAPL energy / power limit / time window units to private dictionary
 **/
void RAPL::initCountersUnits()
{
	if (units.size())
		return; //already initialized
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
		std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
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
#endif


/**
 * read values of the counters (std::string) from the input dictionary and store
 * the values into the dictionary (unsigned long long int)
 **/
#if defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_GEOPM_H
void RAPL::getCounterValues(std::map<std::string, unsigned long long int> & record)
{
	for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
		record[iterator->first] = 0;

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
	}else
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
}
#elif defined _INTEL_ && defined HAVE_X86_ADAPT_H
void RAPL::getCounterValues(std::map<std::string, unsigned long long int> & record)
{
	for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
		record[iterator->first] = 0;
	
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
					continue;
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
				std::cerr << "X86ADAPT RAPL error while closing file descriptor\n";
		}
	}else
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
}
#else
void RAPL::init(bool detailed)
{
	detailedMode = detailed;
	initMap(counters);
	//initCountersUnits() is already called from environmentwrapper
}

double RAPL::getResultValue(unsigned long long int startValue, unsigned long long int stopValue, 
	double runtime, int & cumulative, std::string counter, std::string regionName)
{
	auto it = s_counterDesc.find(counter);
	if(it == s_counterDesc.end())
		return 0.0;

	if(stopValue < startValue)
	{
		return ((it->second.rangeMax - startValue) + stopValue) * it->second.unit;
	}

	return (stopValue - startValue) * it->second.unit;
}

template<typename T>
bool ReadValue(std::ifstream &fs, T &out)
{
	if(!fs.is_open())
		return false;

	fs.seekg(0);
	fs >> out;
	return true;
};

void RAPL::getCounterValues(std::map<std::string, unsigned long long int> & record)
{
	for(const auto &[name, desc]: s_counterDesc)
		record[name] = 0;

	for(auto &[name, desc]: s_counterDesc)
	{
		uint64_t value = 0;
		ReadValue(desc.fs, value);
		record[name] = value;
	}
}
#endif

/**
 * returns label of this class (required for the output)
 **/
std::string RAPL::getSourceLabel()
{
	return "RAPL";
}

/**
 * each counter has a label, that will represent this counter in the output file
 * returns a label of the input counter
 **/
std::string RAPL::getCounterLabel(const std::string counter)
{
	std::map<const std::string, std::string>::iterator it = counterLabel.find(counter);
	if (it != counterLabel.end())
		return it->second;
	else
		return counter;
}

/**
 * returns unit (double) of the requested RAPL counter
 **/
double RAPL::getUnit(std::string counterName)
{
	if (units.count(counterName))
		return units[counterName];
	else
		return -1.0;
}

template<typename T>
bool ReadValue(const std::string &filename, T &out)
{
	std::ifstream fs(filename);
	if(!fs.is_open())
		return false;

	fs >> out;
	return true;
};

/**
 * fill-in the RAPL counters into the dictionary, that will keep values of the required counters
 **/
void RAPL::initMap(std::map<std::string, unsigned long long int> & dict)
{
// fill this vector
	std::vector <std::string> names;

#if defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_GEOPM_H

#ifdef _INTEL_
	names.push_back(RAPL_PCKG_ENERGY);
	names.push_back(RAPL_RAM_ENERGY);
#elif defined _AMD_
	names.push_back(RAPL_PCKG_ENERGY);
	if (detailedMode)
		names.push_back(RAPL_CORE_ENERGY);
	else
		counterLabel[RAPL_PCKG_ENERGY] = RAPL_SUM_ENERGY;
#endif

// in detailed mode RAPL gives us values for each cpu 
// this is a reason for adding a suffix with number of a socket for each counter

// DO NOT EDIT THE REST OF THIS FUNCTION
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
	}else
	{
		for(std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it)
		{
			dict[*it] = 0;
		}
	}
#else

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

			bool state = ReadValue(dieRootPath + pszDomainName + "/name", sysfsName);
			state = state && ReadValue(dieRootPath + pszDomainName + "/max_energy_range_uj", maxCounterRange);

			if(!state)
				continue;

			std::string counterBaseName;

			for(const std::string &name: names)
			{
				if(sysfsName.rfind(name, 0) == 0)
					counterBaseName = name;
			}

			if(counterBaseName.empty())
				continue;

			dict[counterBaseName + "_" + std::to_string(die)] = 0;
			s_counterDesc[counterBaseName + "_" + std::to_string(die)] = SysfsRAPLCounterDesc_t{
				maxCounterRange,
				1.0e-6,
				dieRootPath + pszDomainName + "/energy_uj",
				std::ifstream(dieRootPath + pszDomainName + "/energy_uj")
			};
		}
	}
#endif // SYSFS RAPL
}