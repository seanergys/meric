#include "hwmonwrapper.h"
#include "environmentwrapper.h"

using namespace meric;

struct HWMON::SourceDesc_t {
	enum Domain_t {
		DOMAIN_NONE       = 0,
		DOMAIN_GLOBAL     = 1,
		DOMAIN_PER_SOCKET = 2,
		DOMAIN_PER_CORE   = 3,
		DOMAIN_PER_THREAD = 4,
	};

	enum Flags_t {
		FLAG_NONE_BIT         = 0,
		FLAG_ACCUMULATIVE_BIT = 1 << 0
	};

	enum Type_t {
		TYPE_NONE = 0,
		TYPE_ENERGY,
		TYPE_POWER,
	};

	Type_t   type;
	Domain_t domain;
	uint32_t flags;
	
	double unit;
	std::string label;
	std::string fullName;
	uint64_t rangeMax;

	uint64_t lastValue;
	uint64_t overflowCount;
	std::vector<Sample_t> samples;

	std::string path;
	std::ifstream fs;
};

struct HWMON::CollectSourcesDef_t {
	struct SourceDecl_t {
		SourceDesc_t::Type_t   type;
		SourceDesc_t::Domain_t domain;
		uint32_t 			   flags;
	};

	std::string prefix;
	std::regex  counterFileRegex;
	std::regex  labelRegex;
	std::pair<std::string, std::string> labelFilename;
	std::map<std::string, SourceDecl_t> sources;
};

const std::string HWMON::SUM_ENERGY_LABEL = "Energy consumption [J]";
int HWMON::PROBE_NUMBER;
std::vector<std::string> HWMON::vrSensorLabel;

std::map<std::string, HWMON::SourceDesc_t> HWMON::s_sourceDesc;
std::vector<HWMON::SourceDesc_t *>         HWMON::s_sourceById;

std::chrono::nanoseconds             HWMON::s_overflowCheckPeriod(std::chrono::seconds(10));
std::atomic<bool>		             HWMON::s_sampleStoreEnabled(false);
AsyncSampler<HWMON::CounterRecord_t> HWMON::s_asyncSampler;

void HWMON::init(bool detailed)
{
	initCounters();
	initSamplers();
}

void HWMON::close()
{
	closeSamplers();
}

void HWMON::getCounterValues(CounterRecord_t &record)
{
	s_asyncSampler.Sample(&record);
}

void HWMON::initCounters()
{
	struct SourceDef_t : public CollectSourcesDef_t::SourceDecl_t {
		SourceDef_t(const CollectSourcesDef_t::SourceDecl_t &decl)
			: CollectSourcesDef_t::SourceDecl_t(decl)
		{}

		std::string path;
		std::string baseLabel;
		int id;
		uint64_t period;
	};

	const std::string hwmonRootPath("/sys/class/hwmon");
	std::vector<SourceDef_t> hwmonSources;

#ifdef _AMD_
	{
		std::vector<std::string> hwmonCounterRoots;

		sysfsutils::IterateDir(hwmonRootPath, [&](const std::string &filename) {
			std::string name;
			sysfsutils::ReadValue(hwmonRootPath + "/" + filename + "/name", name);
			if(name == "amd_energy")
			{
				hwmonCounterRoots.push_back(hwmonRootPath + "/" + filename);
			}
		});

		CollectSourcesDef_t collectDesc;
		collectDesc.prefix           = "";
		collectDesc.counterFileRegex = "^energy([0-9]+)_input$";
		collectDesc.labelRegex       = "^([a-zA-Z ]+)([0-9]+)$";
		collectDesc.labelFilename    = { "energy", "_label" };
		collectDesc.sources[HWMON_CORE_ENERGY] = { SourceDesc_t::TYPE_ENERGY, SourceDesc_t::DOMAIN_PER_CORE, SourceDesc_t::FLAG_NONE_BIT };
		collectDesc.sources[HWMON_PCKG_ENERGY] = { SourceDesc_t::TYPE_ENERGY, SourceDesc_t::DOMAIN_PER_SOCKET, SourceDesc_t::FLAG_ACCUMULATIVE_BIT };

		CollectSources(hwmonCounterRoots, collectDesc, hwmonSources, 
			[&](SourceDef_t &out, const std::string &path, 
					const std::smatch &counterMatch, const std::smatch &labelMatch) 
			{
				return true;
			});
		
		int socketCount = 0;
		for(const SourceDef_t &source: hwmonSources)
		{
			socketCount += (source.baseLabel == HWMON_PCKG_ENERGY ? 1 : 0);
		}

		int socketStep = socketCount / Environment::SOCKETS_PER_NODE;
		auto it = hwmonSources.begin();
		while(it != hwmonSources.end())
		{
			if(it->baseLabel == HWMON_PCKG_ENERGY)
			{
				if((it->id % socketStep) != 0)
				{
					it = hwmonSources.erase(it);
					continue;
				}

				it->id = it->id / socketStep;
			}

			++it;
		}
	}
#endif // _AMD_

	{
		std::vector<std::string> hwmonCounterRoots;

		sysfsutils::IterateDir(hwmonRootPath, [&](const std::string &filename) {
			hwmonCounterRoots.push_back(hwmonRootPath + "/" + filename);
		});

		CollectSourcesDef_t collectDesc;
		collectDesc.prefix           = "/device";
		collectDesc.counterFileRegex = "^power([0-9]+)_average$";
		collectDesc.labelRegex       = "^([a-zA-Z ]+)([0-9]+)$";
		collectDesc.labelFilename    = { "power", "_oem_info" };
		collectDesc.sources          = { 
			{ "CPU_Power_Socket", { SourceDesc_t::TYPE_POWER, SourceDesc_t::DOMAIN_PER_SOCKET, SourceDesc_t::FLAG_NONE_BIT } },
			{ "Module_Power_Socket", { SourceDesc_t::TYPE_POWER, SourceDesc_t::DOMAIN_PER_SOCKET, SourceDesc_t::FLAG_NONE_BIT } },
			{ "SysIO_Power_Socket", { SourceDesc_t::TYPE_POWER, SourceDesc_t::DOMAIN_PER_SOCKET, SourceDesc_t::FLAG_NONE_BIT } },
			{ "Grace_Power_Socket", { SourceDesc_t::TYPE_POWER, SourceDesc_t::DOMAIN_PER_SOCKET, SourceDesc_t::FLAG_ACCUMULATIVE_BIT } },
		};

		CollectSources(hwmonCounterRoots, collectDesc, hwmonSources, 
			[&](SourceDef_t &out, const std::string &path, 
					const std::smatch &counterMatch, const std::smatch &labelMatch) 
			{
				int interval = 0;
				sysfsutils::ReadValue(path + "/power" + counterMatch[1].str() + "_average_interval", interval);
				out.period = interval * 1000000ull;
				return true;
			});
	}

	for(const SourceDef_t &source: hwmonSources)
	{
		int suffixLen = 0;
		switch(source.domain)
		{
		case SourceDesc_t::DOMAIN_PER_SOCKET:
			suffixLen = log10(Environment::SOCKETS_PER_NODE) + 1;
			break;
		case SourceDesc_t::DOMAIN_PER_CORE:
			suffixLen = log10(Environment::CORES_PER_SOCKET * Environment::SOCKETS_PER_NODE) + 1;
			break;
		case SourceDesc_t::DOMAIN_PER_THREAD:
			suffixLen = log10(Environment::THREADS_PER_CORE * Environment::CORES_PER_SOCKET * Environment::SOCKETS_PER_NODE) + 1;
			break;
		default:
			break;
		}

		const std::string fullName  = sysfsutils::MakeCounterName(source.baseLabel, counterLabels, {{ source.id, suffixLen }});
		const std::string fullLabel = sysfsutils::MakeCounterName(source.baseLabel, counterLabels, {{ source.id, suffixLen }}, true);

		s_sourceDesc[fullName] = SourceDesc_t{
			source.type,
			source.domain,
			source.flags,

			1.0e-6,
			fullLabel,
			fullName,
			0,
			0,
			0,
			std::vector<Sample_t>(),
			source.path,
			std::ifstream(source.path)
		};
	}
}

double HWMON::getResultValue(CounterValue_t startValue, CounterValue_t stopValue, 
	double runtime, int &cumulative, std::string counter, std::string regionName)
{
	auto it = s_sourceDesc.find(counter);
	if(it == s_sourceDesc.end())
	{
		return 0.0;
	}

	cumulative = ((it->second.flags & SourceDesc_t::FLAG_ACCUMULATIVE_BIT) != 0) ? 1 : 0;
	uint64_t overflow = stopValue.generation - startValue.generation;
	double res = double(stopValue.value) - double(startValue.value);

	return (res + overflow * double(it->second.rangeMax)) * it->second.unit;
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
	auto it = s_sourceDesc.find(counter);
	if(it != s_sourceDesc.end())
	{
		return it->second.label;
	}
	return counter;
}

double HWMON::getUnit(std::string counterName)
{
	auto it = s_sourceDesc.find(counterName);
	if(it != s_sourceDesc.end())
	{
		return it->second.unit;
	}
	return -1.0;
}

void HWMON::start(timespec &time, double sampleFreqHz)
{
	s_sampleStoreEnabled.store(sampleFreqHz != 0.0);

	if(sampleFreqHz != 0)
	{
		s_asyncSampler.SetFrequecy(sampleFreqHz);
	}
	else
	{
		s_asyncSampler.SetPeriod(s_overflowCheckPeriod);
	}
}

void HWMON::stop(timespec &time)
{
	s_sampleStoreEnabled.store(false);
	s_asyncSampler.SetPeriod(s_overflowCheckPeriod);
}

void HWMON::initSamplers()
{
	for(auto &item: s_sourceDesc)
	{
		s_sourceById.push_back(&item.second);
	}

	std::sort(s_sourceById.begin(), s_sourceById.end(),
		[](const SourceDesc_t *pLeft, const SourceDesc_t *pRight) {
			return pLeft->fullName < pRight->fullName;
		});

	// NOTE: Simulates HDEEM Blade sensor, which doesn't exist in HWMON
	s_sourceById.insert(s_sourceById.begin(), nullptr);
	
	for(SourceDesc_t *pSourceDesc: s_sourceById)
	{
		vrSensorLabel.push_back(pSourceDesc ? pSourceDesc->fullName : "");
	}

	PROBE_NUMBER = s_sourceById.size();

	s_asyncSampler.Start(s_overflowCheckPeriod, [](bool isSampling, CounterRecord_t *pRequest) {
		for(auto &desc: s_sourceDesc)
		{
			timespec now;
			clock_gettime(CLOCK_REALTIME, &now);

			uint64_t value = 0;
			sysfsutils::ReadValue(desc.second.fs, value);

			if(isSampling && s_sampleStoreEnabled)
			{
				desc.second.samples.push_back({
					uint64_t(now.tv_sec) * 1000000000ull + now.tv_nsec,
					value
				});
			}

			if(value < desc.second.lastValue && (desc.second.type & SourceDesc_t::TYPE_ENERGY) != 0)
				desc.second.overflowCount++;

			desc.second.lastValue = value;
		}

		if(!pRequest)
			return;

		for(auto &desc: s_sourceDesc)
		{
			if((desc.second.type & SourceDesc_t::TYPE_POWER) != 0)
				continue;
			
			(*pRequest)[desc.first] = CounterValue_t(desc.second.lastValue, desc.second.overflowCount);
		}
	});
}

void HWMON::closeSamplers()
{
	s_asyncSampler.Stop();
}

int HWMON::getSamples()
{
	return 0;
}

void HWMON::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> &samples)
{
	if(sensorId >= s_sourceById.size() || !s_sourceById[sensorId])
	{
		return;
	}

	const SourceDesc_t &desc = *s_sourceById[sensorId];
	samples.reserve(last - first);
	for(size_t i = first; i < last; ++i)
	{
		samples.push_back(double(desc.samples[i].value) * desc.unit);
	}
}

std::string HWMON::getData(double startTime, double duration, 
	size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, 
	double energy[], double minWatts[], double maxWatts[])
{
	uint64_t intervalStart = uint64_t(startTime * 1.0e9);
	uint64_t intervalEnd   = intervalStart + uint64_t(duration * 1.0e9);

	for(size_t i = 0; i < s_sourceById.size(); ++i)
	{
		energy[i]   = 0.0;
		minWatts[i] = 0.0;
		maxWatts[i] = 0.0;

		if(!s_sourceById[i])
		{
			continue;
		}

		const SourceDesc_t &desc = *s_sourceById[i];

		auto startIt = std::lower_bound(desc.samples.begin(), desc.samples.end(), intervalStart, 
			[](const Sample_t &sample, uint64_t value) { return sample.timestamp < value; });
		auto endIt   = std::upper_bound(desc.samples.begin(), desc.samples.end(), intervalEnd, 
			[](uint64_t value, const Sample_t &sample) { return value < sample.timestamp; });

		energy  [i] = 0.0;
		minWatts[i] = std::numeric_limits<double>::max();
		maxWatts[i] = 0.0;
		
		for(auto it = startIt; it != endIt; ++it)
		{
			double power = (double(it->value) * 1.0e-6);
			energy[i]  += power;
			minWatts[i] = std::min(minWatts[i], power);
			maxWatts[i] = std::max(maxWatts[i], power);
		}
		*firstVR = std::distance(desc.samples.begin(), startIt);
		*lastVR  = std::distance(desc.samples.begin(), endIt);
	}

	*firstBlade = *lastBlade = 0;
	return "";
}

template<typename T, typename F>
void HWMON::CollectSources(const std::vector<std::string> &paths, const CollectSourcesDef_t &desc, std::vector<T> &sources, F &&f)
{
	for(const std::string &path: paths)
	{
		sysfsutils::IterateDir(path + desc.prefix, [&](const std::string &filename){
			std::smatch sourceFileMatch;
			if(!std::regex_match(filename, sourceFileMatch, desc.counterFileRegex))
			{
				return;
			}

			std::string hwmonSourceLabel;
			if(!sysfsutils::ReadValue(path + desc.prefix + "/" + desc.labelFilename.first + sourceFileMatch[1].str() + desc.labelFilename.second, hwmonSourceLabel))
			{
				return;
			}

			std::smatch hwmonSourceLabelMatch;
			if(!std::regex_match(hwmonSourceLabel, hwmonSourceLabelMatch, desc.labelRegex))
			{
				return;
			}

			std::string baseLabel = sysfsutils::NormalizeLabel(hwmonSourceLabelMatch[1]);

			auto it = desc.sources.find(baseLabel);
			if(it == desc.sources.end())
				return;

			T source(it->second);
			source.path      = path + desc.prefix + "/" + filename;
			source.baseLabel = baseLabel;
			source.id        = sysfsutils::ParseString<int>(hwmonSourceLabelMatch[2]);

			if(f(source, path + desc.prefix, sourceFileMatch, hwmonSourceLabelMatch))
			{
				sources.push_back(source);
			}
		});
	}
}
