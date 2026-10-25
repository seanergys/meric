#include "nvmlwrapper.h"

#include <cmath>
#include <algorithm>
#include <array>
#include <regex>

#include "environmentwrapper.h"
#include "../basis/sysfsutils.h"

using namespace meric;

#ifdef HAVE_NVML_H
struct NVML::SourceDecl_t {
    std::string name;
    uint32_t fieldId;
    uint32_t scopeId;
	SourceInfo_t info;
};

struct NVML::SourceDesc_t : public SourceDecl_t
{
	SourceDesc_t(const SourceDecl_t &decl, const std::string &label, const std::string &fullName, DeviceDesc_t *pDeviceDesc, double unit = 1.0e-3)
		: SourceDecl_t(decl), unit(unit), label(label), fullName(fullName), lastValue(0), overflowCount(0), lastSampleTime(0), sumValue(0),
		  pDeviceDesc(pDeviceDesc)
	{
	}

	double unit;
    std::string label;
    std::string fullName;

    uint64_t lastValue;
    uint64_t overflowCount;
    std::vector<Sample_t> samples;
	uint64_t lastSampleTime;
	uint64_t sumValue;

	DeviceDesc_t *pDeviceDesc;
};

struct NVML::DeviceDesc_t
{
	nvmlDevice_t device;

	std::vector<nvmlFieldValue_t> fieldValues;
	std::vector<SourceDesc_t *> fieldValuesSources;
	std::vector<SourceDesc_t *> sources;
};

const std::string NVML::SUM_ENERGY_LABEL = "Energy consumption [J]";
int NVML::PROBE_NUMBER;
std::vector<std::string> NVML::vrSensorLabel;

NVML::SourceDescMap_t NVML::s_sourceDesc;
NVML::SourceNameMap_t NVML::s_sourceByName;
NVML::SourceIdVec_t NVML::s_sourceById;
NVML::DeviceDescMap_t NVML::s_deviceDesc;

std::chrono::nanoseconds NVML::s_defaultSamplePeriod(std::chrono::milliseconds(100));
std::chrono::nanoseconds NVML::s_minSamplePeriod(std::chrono::milliseconds(1));
std::chrono::nanoseconds NVML::s_currentSamplePeriod(0);
std::atomic<bool>                   NVML::s_sampleStoreEnabled(false);
AsyncSampler<NVML::CounterRecord_t> NVML::s_asyncSampler;

void NVML::init(bool detailed, double sampleFreqHz)
{
	s_currentSamplePeriod = s_defaultSamplePeriod;

	if (sampleFreqHz > 0.0)
	{
		s_currentSamplePeriod = std::chrono::nanoseconds(uint64_t(1.0e9 / sampleFreqHz));
	}

	if (nvmlInit_v2() != NVML_SUCCESS)
	{
		std::cerr << "NVML ERROR: Unable to initialize\n";
		return;
	}

	initCounters(detailed);
	initSamplers(detailed);
}

void NVML::close()
{
    closeSamplers();
    s_sourceByName.clear();
    s_sourceDesc.clear();
    s_deviceDesc.clear();

    nvmlShutdown();
}

void NVML::getCounterValues(CounterRecord_t &record)
{
	s_asyncSampler.Sample(&record);

	for (auto &desc : s_sourceByName)
	{
        // List only energy counters
		if (desc.second->info.type != SourceInfo_t::TYPE_ENERGY)
		{
			continue;
		}

		unsigned long long energy = 0;
		if (nvmlDeviceGetTotalEnergyConsumption(desc.second->pDeviceDesc->device, &energy) != NVML_SUCCESS)
		{
			energy = 0;
		}

		record[desc.second->fullName] = energy;
    }
}

void NVML::initCounters(bool detailed)
{
	auto IsNVPlatform = []()
	{
		uint32_t deviceCount = 0;
		if (nvmlDeviceGetCount(&deviceCount) != NVML_SUCCESS || deviceCount < 1)
		{
			return false;
		}

		nvmlDevice_t deviceHandle;
		if (nvmlDeviceGetHandleByIndex(0, &deviceHandle) != NVML_SUCCESS)
		{
			return false;
		}

		char name[NVML_DEVICE_NAME_V2_BUFFER_SIZE];
		if (nvmlDeviceGetName(deviceHandle, name, NVML_DEVICE_NAME_V2_BUFFER_SIZE) != NVML_SUCCESS)
		{
			return false;
		}

		std::regex platformRegex("^NVIDIA (GH|GB|VR)[0-9]+.*$");
		return bool(std::regex_match(std::string(name), platformRegex));
	};

	std::vector<SourceDecl_t> sourceDecls;

	// NOTE: "DEVICE" energy counter is disabled due to it's mapping to "nvmlDeviceGetTotalEnergyConsumption", which has uncertain scope.
	//       Energy counters are derived from instant power samples instead.
	if (IsNVPlatform())
	{
		// sourceDecls.push_back({ "DEVICE",     0,                         0, { SourceInfo_t::TYPE_ENERGY, SourceInfo_t::DOMAIN_PER_SOCKET,
		// SourceInfo_t::FLAG_NONE_BIT }});
		sourceDecls.push_back({"GPU", NVML_FI_DEV_POWER_INSTANT, 0, {SourceInfo_t::TYPE_POWER, SourceInfo_t::DOMAIN_PER_SOCKET, SourceInfo_t::FLAG_NONE_BIT}});
		sourceDecls.push_back(
			{"MODULE", NVML_FI_DEV_POWER_INSTANT, 1, {SourceInfo_t::TYPE_POWER, SourceInfo_t::DOMAIN_PER_SOCKET, SourceInfo_t::FLAG_ACCUMULATIVE_BIT}});
		sourceDecls.push_back(
			{"GPU_MEMORY", NVML_FI_DEV_POWER_AVERAGE, 2, {SourceInfo_t::TYPE_POWER, SourceInfo_t::DOMAIN_PER_SOCKET, SourceInfo_t::FLAG_NONE_BIT}});
	}
	else
    {
		// sourceDecls.push_back({ "DEVICE", 0,                         0, { SourceInfo_t::TYPE_ENERGY, SourceInfo_t::DOMAIN_PER_SOCKET,
		// SourceInfo_t::FLAG_NONE_BIT }});
		sourceDecls.push_back(
			{"GPU", NVML_FI_DEV_POWER_INSTANT, 0, {SourceInfo_t::TYPE_POWER, SourceInfo_t::DOMAIN_PER_SOCKET, SourceInfo_t::FLAG_ACCUMULATIVE_BIT}});
	}

	uint32_t deviceCount = 0;

	if (nvmlDeviceGetCount(&deviceCount) != NVML_SUCCESS)
	{
		return;
	}

	int suffixLen = log10(deviceCount) + 1;

	for (uint32_t i = 0; i < deviceCount; ++i)
	{
		nvmlDevice_t deviceHandle;
		if (nvmlDeviceGetHandleByIndex(i, &deviceHandle) != NVML_SUCCESS)
		{
			continue;
		}

		auto deviceDescRes = s_deviceDesc.insert(std::make_pair(i, DeviceDesc_t{deviceHandle}));
		DeviceDesc_t &deviceDesc = deviceDescRes.first->second;

		for (const SourceDecl_t &sourceDecl : sourceDecls)
		{
			const std::string fullName = sysfsutils::MakeCounterName(sourceDecl.name, "", "", {{i, suffixLen}});
			const std::string fullLabel = sysfsutils::MakeCounterName(sourceDecl.name, "", "", {{i, suffixLen}});

			SourceDesc_t sourceDesc(sourceDecl, fullLabel, fullName, &deviceDesc);

			auto baseRes = s_sourceDesc.insert(std::make_pair(sourceDecl.name, std::map<int, SourceDesc_t>()));
			auto res = baseRes.first->second.insert(std::make_pair(i, std::move(sourceDesc)));

			s_sourceByName[fullName] = &res.first->second;

			if (sourceDecl.info.type == SourceInfo_t::TYPE_POWER)
			{
				deviceDesc.fieldValues.push_back(nvmlFieldValue_t{});
                deviceDesc.fieldValues.back().fieldId = sourceDecl.fieldId;
                deviceDesc.fieldValues.back().scopeId = sourceDecl.scopeId;
				deviceDesc.fieldValuesSources.push_back(&res.first->second);
			}
			else if (sourceDecl.info.type == SourceInfo_t::TYPE_ENERGY)
			{
				deviceDesc.sources.push_back(&res.first->second);
			}
		}
	}
}

double NVML::getResultValue(CounterValue_t startValue, CounterValue_t stopValue, double runtime, int &cumulative, std::string counter, std::string regionName)
{
	auto it = s_sourceByName.find(counter);
	if (it == s_sourceByName.end())
	{
		return 0.0;
	}

	SourceDesc_t &desc = *it->second;

	cumulative = desc.info.IsAccumulative() ? 1 : 0;
	return double(stopValue - startValue) * getUnit(counter);
}

std::string NVML::summaryCounter()
{
    return "ENERGY_CONSUMPTION";
}

std::string NVML::getSummaryCounterName()
{
    return SUM_ENERGY_LABEL;
}

std::string NVML::getSourceLabel()
{
    return "NVML";
}

std::string NVML::getCounterLabel(const std::string &counter)
{
    auto it = s_sourceByName.find(counter);
	if (it != s_sourceByName.end())
	{
		return it->second->label + " [J]";
	}

	return counter + " [J]";
}

double NVML::getUnit(std::string counterName)
{
    auto it = s_sourceByName.find(counterName);
	if (it != s_sourceByName.end())
	{
		return it->second->unit;
	}

	return -1.0;
}

void NVML::start(timespec &time, bool storeSamples, double sampleFreqHz)
{
	if (sampleFreqHz > 0.0)
	{
		s_currentSamplePeriod = std::max(s_minSamplePeriod, std::chrono::nanoseconds(uint64_t(1.0e9 / sampleFreqHz)));
		s_asyncSampler.SetPeriod(s_currentSamplePeriod);
	}

	s_sampleStoreEnabled.store(storeSamples);
}

void NVML::stop(timespec &time)
{
    s_sampleStoreEnabled.store(false);
}

void NVML::initSamplers(bool detailed)
{
	for (auto &desc : s_sourceByName)
	{
		s_sourceById.push_back(desc.second);
	}

	std::sort(s_sourceById.begin(), s_sourceById.end(),
			  [](const SourceDesc_t *pLeft, const SourceDesc_t *pRight) { return pLeft->fullName < pRight->fullName; });

	// NOTR: Simulates HDEEM Blade sensor, which doesn't exist in NVML
	s_sourceById.insert(s_sourceById.begin(), nullptr);

	vrSensorLabel.clear();
	for (SourceDesc_t *pSourceDesc : s_sourceById)
	{
		vrSensorLabel.push_back(pSourceDesc ? pSourceDesc->fullName : "");
	}

	PROBE_NUMBER = s_sourceById.size();

	s_asyncSampler.Start(s_currentSamplePeriod,
						 [](bool isSampling, CounterRecord_t *pRequest)
						 {
							 auto ProcessSampleValue = [&](SourceDesc_t *pSource, uint64_t timestamp, uint64_t value)
							 {
								 if (isSampling && s_sampleStoreEnabled)
								 {
									 pSource->samples.push_back({timestamp, value});
								 }

								 if (isSampling)
								 {
									 if (pSource->lastSampleTime > 0)
									 {
										 uint64_t deltaTime = timestamp - pSource->lastSampleTime;
										 pSource->sumValue += uint64_t(value * (double(deltaTime) * 1.0e-9));
									 }
									 pSource->lastSampleTime = timestamp;
								 }
							 };

							 for (auto &deviceDescIt : s_deviceDesc)
							 {
								 timespec now;
								 clock_gettime(CLOCK_REALTIME, &now);
								 const uint64_t timestamp = uint64_t(now.tv_sec) * 1000000000ull + now.tv_nsec;

								 {
									 auto &fieldValues = deviceDescIt.second.fieldValues;
									 auto &sources = deviceDescIt.second.fieldValuesSources;

									 nvmlReturn_t ret = nvmlDeviceGetFieldValues(deviceDescIt.second.device, fieldValues.size(), fieldValues.data());

									 for (size_t i = 0; i < fieldValues.size(); ++i)
									 {
										 uint64_t value = 0;
										 if (ret == NVML_SUCCESS && fieldValues[i].nvmlReturn == NVML_SUCCESS)
										 {
											 value = fieldValues[i].value.uiVal;
											 if (value < sources[i]->lastValue && sources[i]->info.type == SourceInfo_t::TYPE_ENERGY)
											 {
												 sources[i]->overflowCount++;
											 }
											 sources[i]->lastValue = value;
										 }
										 ProcessSampleValue(sources[i], timestamp, value);
									 }
								 }

								 {
									 auto &sources = deviceDescIt.second.sources;
									 for (auto &pSource : sources)
									 {
										 unsigned long long value = 0;
										 if (nvmlDeviceGetTotalEnergyConsumption(pSource->pDeviceDesc->device, &value) != NVML_SUCCESS)
										 {
											 value = 0;
										 }
										 ProcessSampleValue(pSource, timestamp, value);
									 }
								 }
							 }

							 if (!pRequest)
							 {
								 return;
							 }

							 for (auto &desc : s_sourceByName)
							 {
								 if (desc.second->info.type == SourceInfo_t::TYPE_POWER)
								 {
									 (*pRequest)[desc.second->fullName] = CounterValue_t(desc.second->sumValue);
								 }
								 else
								 {
									 (*pRequest)[desc.second->fullName] = CounterValue_t(desc.second->lastValue);
								 }
							 }
						 });
}

void NVML::closeSamplers()
{
    s_asyncSampler.Stop();
    s_sourceById.clear();

	StreamStats stats;
	s_asyncSampler.GetSampleStats(stats);
	MERIC_INFO << "INFO: [NVML/Sampling] Collected " << stats.GetCount() << " samples (MIN: " << stats.GetMin() << ", MAX: " << stats.GetMax()
			   << ", AVG: " << stats.GetMean() << ") [s]" << std::endl;
}

int NVML::getSamples()
{
    return 0;
}

void NVML::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> &samples)
{
	if (sensorId >= s_sourceById.size() || !s_sourceById[sensorId])
	{
		return;
	}

	const SourceDesc_t &desc = *s_sourceById[sensorId];
    samples.reserve(last - first);
	for (size_t i = first; i < last; ++i)
	{
		if (desc.info.type == SourceInfo_t::TYPE_POWER)
		{
			samples.push_back(double(desc.samples[i].value) * desc.unit);
		}
		else if (desc.info.type == SourceInfo_t::TYPE_ENERGY)
		{
			samples.push_back(PowerFromEnergySamples(desc.samples[i], std::numeric_limits<uint64_t>::max(), i > 0 ? &desc.samples[i - 1] : nullptr) *
							  desc.unit);
		}
	}
}

std::string NVML::getData(double startTime, double duration, SamplerRecord_t &record)
{
	const uint64_t intervalStart = uint64_t(startTime * 1.0e9);
	const uint64_t intervalEnd = intervalStart + uint64_t(duration * 1.0e9);

	for (size_t i = 0; i < s_sourceById.size(); ++i)
	{
		if (!s_sourceById[i])
		{
			continue;
		}

		const SourceDesc_t &desc = *s_sourceById[i];

		record.push_back(SamplerData_t(desc.info));
		SamplerData_t &data = record.back();
		data.id = i;
		data.name = desc.fullName;
		data.energy = 0.0;
		data.min = std::numeric_limits<double>::max();
		data.max = 0.0;

		auto startIt = std::lower_bound(desc.samples.begin(), desc.samples.end(), intervalStart,
										[](const Sample_t &sample, uint64_t value) { return sample.timestamp < value; });
		auto endIt = std::upper_bound(desc.samples.begin(), desc.samples.end(), intervalEnd,
									  [](uint64_t value, const Sample_t &sample) { return value < sample.timestamp; });

		for (auto it = startIt; it != endIt; ++it)
		{
			double power = 0.0;

			if (desc.info.type == SourceInfo_t::TYPE_POWER)
			{
				power = (double(it->value) * desc.unit);
			}
			else if (desc.info.type == SourceInfo_t::TYPE_ENERGY)
			{
				if (it == desc.samples.begin())
				{
					continue;
				}
				power = PowerFromEnergySamples(*it, std::numeric_limits<uint64_t>::max(), (it - 1).base()) * desc.unit;
			}

			data.energy += power;
			data.min = std::min(data.min, power);
			data.max = std::max(data.max, power);
		}

		data.first = std::distance(desc.samples.begin(), startIt);
		data.last = std::distance(desc.samples.begin(), endIt);

		if (data.min > data.max)
		{
			data.min = data.max;
		}
	}

	return "";
}

void NVML::getSamplePeriodInfo(SamplePeriodInfo_t &info)
{
	info.min = s_minSamplePeriod.count();
	info.initial = s_defaultSamplePeriod.count();
	info.current = s_currentSamplePeriod.count();
}

bool NVML::isAvailable()
{
	if (nvmlInit_v2() != NVML_SUCCESS)
	{
		return false;
	}

	uint32_t deviceCount = 0;
	unsigned long long energy;
	nvmlDevice_t deviceHandle;

	bool available = (nvmlDeviceGetCount(&deviceCount) == NVML_SUCCESS) && deviceCount > 0;
	available = available && (nvmlDeviceGetHandleByIndex(0, &deviceHandle) == NVML_SUCCESS);
	available = available && (nvmlDeviceGetTotalEnergyConsumption(deviceHandle, &energy) == NVML_SUCCESS);

	nvmlShutdown();
	return available;
}

nvmlDevice_t NVML::getGPUHandle(int id)
{
    auto it = s_deviceDesc.find(id);
	if (it == s_deviceDesc.end())
	{
		return nvmlDevice_t{};
	}
	return it->second.device;
}

size_t NVML::getGPUsPerNode()
{
    return s_deviceDesc.size();
}

#else

struct NVML::SourceDecl_t
{
};

struct NVML::SourceDesc_t
{
};

struct NVML::DeviceDesc_t
{
};

const std::string NVML::SUM_ENERGY_LABEL = "Energy consumption [J]";
int NVML::PROBE_NUMBER;
std::vector<std::string> NVML::vrSensorLabel;

NVML::SourceDescMap_t NVML::s_sourceDesc;
NVML::SourceNameMap_t NVML::s_sourceByName;
NVML::SourceIdVec_t NVML::s_sourceById;
NVML::DeviceDescMap_t NVML::s_deviceDesc;

std::chrono::nanoseconds NVML::s_defaultSamplePeriod(std::chrono::milliseconds(100));
std::chrono::nanoseconds NVML::s_minSamplePeriod(std::chrono::milliseconds(100));
std::chrono::nanoseconds NVML::s_currentSamplePeriod(0);
std::atomic<bool> NVML::s_sampleStoreEnabled(false);
AsyncSampler<NVML::CounterRecord_t> NVML::s_asyncSampler;

void NVML::init(bool detailed, double sampleFreqHz)
{
    // nvmlInit_v2();

    initCounters(detailed);
    initSamplers(detailed);
}

void NVML::close()
{
    closeSamplers();
    // nvmlShutdown();
}

void NVML::getCounterValues(CounterRecord_t &record)
{
}

void NVML::initCounters(bool detailed)
{
}

double NVML::getResultValue(CounterValue_t startValue, CounterValue_t stopValue, double runtime, int &cumulative, std::string counter, std::string regionName)
{
    return 0.0;
}

std::string NVML::summaryCounter()
{
    return "ENERGY_CONSUMPTION";
}

std::string NVML::getSummaryCounterName()
{
    return SUM_ENERGY_LABEL;
}

std::string NVML::getSourceLabel()
{
    return "NVML";
}

std::string NVML::getCounterLabel(const std::string &counter)
{
    return "";
}

double NVML::getUnit(std::string counterName)
{
    return -1.0;
}

void NVML::start(timespec &time, bool storeSamples, double sampleFreqHz) {}

void NVML::stop(timespec &time)
{
}

void NVML::initSamplers(bool detailed)
{
}

void NVML::closeSamplers()
{
}

int NVML::getSamples()
{
    return 0;
}

void NVML::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> &samples)
{
}

std::string NVML::getData(double startTime, double duration, SamplerRecord_t &record) { return ""; }

void NVML::getSamplePeriodInfo(SamplePeriodInfo_t &info) { info = SamplePeriodInfo_t(); }

bool NVML::isAvailable()
{
    return false;
}

nvmlDevice_t NVML::getGPUHandle(int id)
{
}

size_t NVML::getGPUsPerNode()
{
    return 0;
}

#endif // HAVE_NVML_H
