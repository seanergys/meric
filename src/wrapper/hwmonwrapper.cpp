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

const std::string HWMON::SUM_ENERGY_LABEL = "Energy consumption [J]";
int HWMON::PROBE_NUMBER;
std::vector<std::string> HWMON::vrSensorLabel;

std::map<std::string, HWMON::CounterDesc_t> HWMON::s_counterDesc;
std::map<std::string, HWMON::SamplerDesc_t> HWMON::s_samplerDesc;
std::vector<HWMON::SamplerDesc_t *>         HWMON::s_samplerById;

uint64_t          HWMON::s_samplingPeriod(1000000000ull);
std::thread       HWMON::s_samplingThread;
std::atomic<bool> HWMON::s_samplingEnabled(true);

void HWMON::init(bool detailed)
{
	initCounters();
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

void HWMON::initCounters()
{
	struct Counter_t {
		std::string path;
		std::string baseLabel;
		int id;
	};

	const std::string hwmonRootPath("/sys/class/hwmon");
	std::vector<Counter_t> hwmonCounters;
	std::map<std::string, int> labelsSuffixLen;
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

		CollectCountersDesc_t collectDesc;
		collectDesc.prefix           = "";
		collectDesc.counterFileRegex = "^energy([0-9]+)_input$";
		collectDesc.labelRegex       = "^([a-zA-Z ]+)([0-9]+)$";
		collectDesc.labelFilename    = { "energy", "_label" };
		collectDesc.counters         = { HWMON_CORE_ENERGY, HWMON_PCKG_ENERGY };

		CollectCounters(hwmonCounterRoots, collectDesc, hwmonCounters, 
			[&](Counter_t &out, const std::string &path, 
					const std::smatch &counterMatch, const std::smatch &labelMatch) 
			{
				// out.samplePeriod = 0.0;
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

	for(const Counter_t &counter: hwmonCounters)
	{
		const std::string fullName  = sysfsutils::MakeCounterName(counter.baseLabel, counterLabels, {{ counter.id, labelsSuffixLen[counter.baseLabel] }});
		const std::string fullLabel = sysfsutils::MakeCounterName(counter.baseLabel, counterLabels, {{ counter.id, labelsSuffixLen[counter.baseLabel] }}, true);

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

void HWMON::start(timespec &time)
{
	initSamplers();
}

void HWMON::stop(timespec &time)
{
	closeSamplers();
}

void HWMON::initSamplers()
{
	struct Sampler_t {
		std::string path;
		std::string baseLabel;
		int id;
		uint64_t period;
	};

	const std::string hwmonRootPath("/sys/class/hwmon");
	std::vector<Sampler_t> hwmonCounters;
	std::map<std::string, int> labelsSuffixLen;

	{
		std::vector<std::string> hwmonCounterRoots;

		sysfsutils::IterateDir(hwmonRootPath, [&](const std::string &filename) {
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
			[&](Sampler_t &out, const std::string &path, 
					const std::smatch &counterMatch, const std::smatch &labelMatch) 
			{
				int interval = 0;
				sysfsutils::ReadValue(path + "/power" + counterMatch[1].str() + "_average_interval", interval);
				out.period = interval * 1000000ull;
				return true;
			});
	}

	s_samplerById.push_back(NULL);

	for(const Sampler_t &sampler: hwmonCounters)
	{
		const std::string fullName  = sysfsutils::MakeCounterName(sampler.baseLabel, counterLabels, {{ sampler.id, labelsSuffixLen[sampler.baseLabel] }});
		const std::string fullLabel = sysfsutils::MakeCounterName(sampler.baseLabel, counterLabels, {{ sampler.id, labelsSuffixLen[sampler.baseLabel] }}, true);

		s_samplerDesc[fullName] = SamplerDesc_t{
			s_samplerById.size(),
			1.0e-6,
			sampler.path,
			std::ifstream(sampler.path),
			fullLabel,
			(sampler.baseLabel == "Grace_Power_Socket"),
			sampler.period,
		};

		s_samplerById.push_back(&s_samplerDesc[fullName]);
		if(sampler.period != 0)
		{
			s_samplingPeriod = std::min(s_samplingPeriod, sampler.period);
		}
	}

	std::sort(s_samplerById.begin(), s_samplerById.end(),
		[](const SamplerDesc_t *pLeft, const SamplerDesc_t *pRight) {
			if(!pLeft)
			{
				return true;
			}
			if(!pRight)
			{
				return false;
			}
			return pLeft->label < pRight->label;
		});
	
	for(const SamplerDesc_t *pSamplerDesc: s_samplerById)
	{
		if(!pSamplerDesc)
		{
			vrSensorLabel.push_back("");
			continue;
		}
		vrSensorLabel.push_back(pSamplerDesc->label);
	}

	PROBE_NUMBER = s_samplerById.size();

	s_samplingPeriod = s_samplingPeriod / 2;

	if(s_samplingPeriod < 1000000ull)
	{
		s_samplingPeriod = 1000000ull;
	}

	s_samplingThread = std::thread([]() {
		timespec time;
		clock_gettime(CLOCK_REALTIME, &time);

		while(s_samplingEnabled.load())
		{
			for(auto &desc: s_samplerDesc)
			{
				timespec now;
				clock_gettime(CLOCK_REALTIME, &now);
				uint64_t value = 0;
				sysfsutils::ReadValue(desc.second.fs, value);

				desc.second.samples.push_back({
					uint64_t(now.tv_sec) * 1000000000ull + now.tv_nsec,
					value
				});
			}

			{
				while(clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &time, NULL) != 0) {}
				time.tv_nsec += s_samplingPeriod;
				if(time.tv_nsec >= 1000000000ull)
				{
					time.tv_nsec -= 1000000000ull;
					time.tv_sec++;
				}
			}
		}
	});
}

void HWMON::closeSamplers()
{
	s_samplingEnabled.store(false);
	if(s_samplingThread.joinable())
	{
		s_samplingThread.join();
	}

	// Check for number of "0" samples in the sampled data
	/*for(const auto &samplerDesc: s_samplerDesc)
	{
		const SamplerDesc_t &desc = samplerDesc.second;
		
		size_t zeroSamples = 0;
		for(const auto &sample: desc.samples)
		{
			zeroSamples += (sample.value == 0) ? 1 : 0;
		}
		
		std::cout << "ZERO_SAMPLES: " << desc.label << " " << double(zeroSamples) / desc.samples.size() << std::endl;
	}*/
}

int HWMON::getSamples()
{
	return 0;
}

void HWMON::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> &samples)
{
	if(sensorId >= s_samplerById.size() || !s_samplerById[sensorId])
	{
		return;
	}

	const SamplerDesc_t &desc = *s_samplerById[sensorId];
	samples.reserve(last - first);
	for(size_t i = first; i < last; ++i)
	{
		samples.push_back(double(desc.samples[i].value) * 1.0e-6);
	}
}

std::string HWMON::getData(double startTime, double duration, 
	size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, 
	double energy[], double minWatts[], double maxWatts[])
{
	uint64_t intervalStart = uint64_t(startTime * 1.0e9);
	uint64_t intervalEnd   = intervalStart + uint64_t(duration * 1.0e9);

	for(size_t i = 0; i < s_samplerById.size(); ++i)
	{
		energy[i]   = 0.0;
		minWatts[i] = 0.0;
		maxWatts[i] = 0.0;

		if(!s_samplerById[i])
		{
			continue;
		}

		const SamplerDesc_t &desc = *s_samplerById[i];

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
			counter.path      = path + desc.prefix + "/" + filename;
			counter.baseLabel = baseLabel;
			counter.id        = sysfsutils::ParseString<int>(hwmonCounterLabelMatch[2]);

			if(f(counter, path + desc.prefix, counterFileMatch, hwmonCounterLabelMatch))
			{
				counters.push_back(counter);
			}
		});
	}
}
