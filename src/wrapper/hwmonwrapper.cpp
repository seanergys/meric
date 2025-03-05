#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <cmath>
#include <dirent.h>

#include "hwmonwrapper.h"
#include "environmentwrapper.h"

using namespace meric;

struct CollectCountersDesc_t {
	std::string prefix;
	std::regex  counterFileRegex;
	std::regex  labelRegex;
	std::pair<std::string, std::string> labelFilename;
	std::set<std::string> counters;
};

template<typename T, typename F>
void CollectCounters(const std::vector<std::string> &paths, const CollectCountersDesc_t &desc, std::vector<T> &counters, F &&f);

std::map<std::string, HWMON::CounterDesc_t> HWMON::s_counterDesc;
std::map<std::string, unsigned long long int> HWMON::counters;

void HWMON::init(bool detailed)
{
	initMap(counters);
}

void HWMON::close()
{
}

void HWMON::getCounterValues(std::map<std::string, unsigned long long int> &record)
{
	for(auto &desc: s_counterDesc)
	{
		uint64_t value = 0;
		sysfsutils::ReadValue(desc.second.fs, value);
		record[desc.first] = value;
	}
}

void HWMON::initMap(std::map<std::string, unsigned long long int> &dict)
{
	struct Counter_t {
		std::string path;
		std::string baseLabel;
		int id;
		double samplePeriod;
	};

	const std::string hwmonRootPath("/sys/class/hwmon");
	std::vector<Counter_t> hwmonCounters;
	std::map<std::string, int> labelsSuffixLen;

#ifdef _AMD_
	{
		std::vector<std::string> hwmonCounterRoots;

		IterateDir(hwmonRootPath, [&](const std::string &filename) {
			std::string name;
			sysfsutils::ReadValue(hwmonRootPath + "/" + filename + "/name", name);
			if(name == "amd_energy")
			{
				hwmonCounterRoots.push_back(hwmonRootPath + "/" + filename);
			}
		});

		CollectCountersDesc_t collectDesc;
		collectDesc.prefix           = "";
		collectDesc.counterFileRegex = "^energy([0-9]+)_input$";
		collectDesc.labelRegex       = "^([a-zA-Z ]+)([0-9]+)$";
		collectDesc.labelFilename    = { "energy", "_label" };
		collectDesc.counters         =
//#ifdef _AMD_		
			{ HWMON_CORE_ENERGY, HWMON_PCKG_ENERGY };
//		#else
//			{};
//		#endif

		CollectCounters(hwmonCounterRoots, collectDesc, hwmonCounters, 
			[&](Counter_t &out, const std::string &path, 
					const std::smatch &counterMatch, const std::smatch &labelMatch) 
			{
				out.samplePeriod = 0.0;
				return true;
			});
		
		labelsSuffixLen[HWMON_CORE_ENERGY] = log10(Environment::CORES_PER_SOCKET * Environment::SOCKETS_PER_NODE) + 1;
		labelsSuffixLen[HWMON_PCKG_ENERGY] = log10(Environment::SOCKETS_PER_NODE) + 1;
		
		int socketCount = 0;
		for(const Counter_t &counter: hwmonCounters)
		{
			socketCount += (counter.baseLabel == HWMON_PCKG_ENERGY ? 1 : 0);
		}

		int socketStep = socketCount / Environment::SOCKETS_PER_NODE;
		auto it = hwmonCounters.begin();
		while(it != hwmonCounters.end())
		{
			if(it->baseLabel == HWMON_PCKG_ENERGY)
			{
				if((it->id % socketStep) != 0)
				{
					it = hwmonCounters.erase(it);
					continue;
				}
				it->id = it->id / socketStep;
			}
			++it;
		}
	}
#endif

	/*{
		std::vector<std::string> hwmonCounterRoots;

		IterateDir(hwmonRootPath, [&](const std::string &filename) {
			hwmonCounterRoots.push_back(hwmonRootPath + "/" + filename);
		});

		CollectCountersDesc_t collectDesc;
		collectDesc.prefix           = "/device";
		collectDesc.counterFileRegex = "^power([0-9]+)_average$";
		collectDesc.labelRegex       = "^([a-zA-Z ]+)([0-9]+)$";
		collectDesc.labelFilename    = { "power", "_oem_info" };
		collectDesc.counters         = { 
			"CPU_Power_Socket", "Module_Power_Socket", 
			"SysIO_Power_Socket", "Grace_Power_Socket" 
		};

		CollectCounters(hwmonCounterRoots, collectDesc, hwmonCounters, 
			[&](Counter_t &out, const std::string &path, 
					const std::smatch &counterMatch, const std::smatch &labelMatch) 
			{
				int interval = 0;
				sysfsutils::ReadValue(path + "/power" + counterMatch[1].str() + "_average_interval", interval);

				out.samplePeriod = interval * 1.0e-3;

				return true;
			});
	}*/

#ifdef _AMD_
	for(const Counter_t &counter: hwmonCounters)
	{
		const std::string fullName  = MakeCounterName(counter.baseLabel, counterLabels, {{ counter.id, labelsSuffixLen[counter.baseLabel] }});
		const std::string fullLabel = MakeCounterName(counter.baseLabel, counterLabels, {{ counter.id, labelsSuffixLen[counter.baseLabel] }}, true);

		s_counterDesc[fullName] = CounterDesc_t{
			1.0e-6,
			counter.path,
			std::ifstream(counter.path),
			fullLabel,
			counter.baseLabel == HWMON_PCKG_ENERGY
		};
	}
#endif
}

double HWMON::getResultValue(unsigned long long int startValue, unsigned long long int stopValue, 
	double runtime, int &cumulative, std::string counter, std::string regionName)
{
	auto it = s_counterDesc.find(counter);
	if(it == s_counterDesc.end())
	{
		return 0.0;
	}
	cumulative = it->second.isCumulative ? 1 : 0;

	// TODO: Handle counter overflow.
	return (stopValue - startValue) * it->second.unit;
}

std::string HWMON::summaryCounter()
{
	return HWMON_SUM_ENERGY;
}

std::string HWMON::getSummaryCounterName()
{
	auto it = counterLabels.find(summaryCounter());
	if(it != counterLabels.end())
	{
		return it->second.first + " " + it->second.second;
	}
	return summaryCounter();
}

std::string HWMON::getSourceLabel()
{
	return "HWMON";
}

std::string HWMON::getCounterLabel(const std::string counter)
{
	auto it = s_counterDesc.find(counter);
	if(it != s_counterDesc.end())
	{
		return it->second.label;
	}
	return counter;
}

double HWMON::getUnit(std::string counterName)
{
	auto it = s_counterDesc.find(counterName);
	if(it != s_counterDesc.end())
	{
		return it->second.unit;
	}
	return -1.0;
}

template<typename T, typename F>
void CollectCounters(const std::vector<std::string> &paths, const CollectCountersDesc_t &desc, std::vector<T> &counters, F &&f)
{
	for(const std::string &path: paths)
	{
		sysfsutils::IterateDir(path + desc.prefix, [&](const std::string &filename){
			std::smatch counterFileMatch;
			if(!std::regex_match(filename, counterFileMatch, desc.counterFileRegex))
			{
				return;
			}

			std::string hwmonCounterLabel;
			if(!sysfsutils::ReadValue(path + desc.prefix + "/" + desc.labelFilename.first + counterFileMatch[1].str() + desc.labelFilename.second, hwmonCounterLabel))
			{
				return;
			}

			std::smatch hwmonCounterLabelMatch;
			if(!std::regex_match(hwmonCounterLabel, hwmonCounterLabelMatch, desc.labelRegex))
			{
				return;
			}

			std::string baseLabel = sysfsutils::NormalizeLabel(hwmonCounterLabelMatch[1]);

			if(desc.counters.count(baseLabel) < 1)
			{
				return;
			}

			T counter;
			counter.path      = path + "/" + filename;
			counter.baseLabel = baseLabel;
			counter.id        = sysfsutils::ParseString<int>(hwmonCounterLabelMatch[2]);

			if(f(counter, path + desc.prefix, counterFileMatch, hwmonCounterLabelMatch))
			{
				counters.push_back(counter);
			}
		});
	}
}
