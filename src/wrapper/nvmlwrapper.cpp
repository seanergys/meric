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
    enum Type_t {
        TYPE_NONE = 0,
        TYPE_ENERGY,
        TYPE_POWER,
    };

    std::string name;
    Type_t type;
    uint32_t fieldId;
    uint32_t scopeId;
};

struct NVML::SourceDesc_t {
    SourceDesc_t(const SourceDecl_t &decl, 
        const std::string &label, const std::string &fullName, 
        DeviceDesc_t *pDeviceDesc, double unit = 1.0e-3)
        : type(decl.type)
        , unit(unit)
        , label(label)
        , fullName(fullName)
        , lastValue(0)
        , overflowCount(0)
        , pDeviceDesc(pDeviceDesc)
    {}

    SourceDecl_t::Type_t type;
    double unit;
    std::string label;
    std::string fullName;

    uint64_t lastValue;
    uint64_t overflowCount;
    std::vector<Sample_t> samples;

    DeviceDesc_t *pDeviceDesc;
};

struct NVML::DeviceDesc_t {
    nvmlDevice_t device;

    std::vector<nvmlFieldValue_t> fieldValues;
    std::vector<SourceDesc_t *>   sources;
};

const std::string NVML::SUM_ENERGY_LABEL = "Energy consumption [J]";
int NVML::PROBE_NUMBER;
std::vector<std::string> NVML::vrSensorLabel;

NVML::SourceDescMap_t NVML::s_sourceDesc;
NVML::SourceNameMap_t NVML::s_sourceByName;
NVML::SourceIdVec_t   NVML::s_sourceById;
NVML::DeviceDescMap_t NVML::s_deviceDesc;

std::chrono::nanoseconds              NVML::s_overflowCheckPeriod(std::chrono::seconds(10));
std::atomic<bool>                      NVML::s_sampleStoreEnabled(false);
AsyncSampler<NVML::CounterRecord_t> NVML::s_asyncSampler;

void NVML::init(bool detailed)
{
    if(nvmlInit_v2() != NVML_SUCCESS)
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

    nvmlShutdown();
}

void NVML::getCounterValues(CounterRecord_t &record)
{
    for(auto &desc: s_sourceByName)
    {
        // List only energy counters
        if(desc.second->type != SourceDecl_t::TYPE_ENERGY)
        {
            continue;
        }

        unsigned long long energy = 0;
        if(nvmlDeviceGetTotalEnergyConsumption(desc.second->pDeviceDesc->device, &energy) != NVML_SUCCESS)
        {
            energy = 0;
        }

        record[desc.second->fullName] = energy;
    }
}

void NVML::initCounters(bool detailed)
{
    auto IsNVPlatform = []() {
        uint32_t deviceCount = 0;
        if(nvmlDeviceGetCount(&deviceCount) != NVML_SUCCESS || deviceCount < 1) {
            return false;
        }

        nvmlDevice_t deviceHandle;
        if(nvmlDeviceGetHandleByIndex(0, &deviceHandle) != NVML_SUCCESS) {
            return false;
        }

        char name[NVML_DEVICE_NAME_V2_BUFFER_SIZE];
        if(nvmlDeviceGetName(deviceHandle, name, NVML_DEVICE_NAME_V2_BUFFER_SIZE) != NVML_SUCCESS) {
            return false;
        }

        std::regex platformRegex("^NVIDIA (GH|GB|VR)[0-9]+.*$");
        return bool(std::regex_match(std::string(name), platformRegex));
    };

    std::vector<SourceDecl_t> sourceDecls;

    if(IsNVPlatform())
    {
        sourceDecls.push_back({ "MODULE_ENERGY", SourceDecl_t::TYPE_ENERGY,                        0, 0 });
        sourceDecls.push_back({ "GPU_POWER",     SourceDecl_t::TYPE_POWER, NVML_FI_DEV_POWER_INSTANT, 0 });
        sourceDecls.push_back({ "MODULE_POWER",  SourceDecl_t::TYPE_POWER, NVML_FI_DEV_POWER_INSTANT, 1 });
        sourceDecls.push_back({ "MEMORY_POWER",  SourceDecl_t::TYPE_POWER, NVML_FI_DEV_POWER_AVERAGE, 2 });
    }
    else
    {
        sourceDecls.push_back({ "GPU_ENERGY", SourceDecl_t::TYPE_ENERGY,                        0, 0 });
        sourceDecls.push_back({ "GPU_POWER",  SourceDecl_t::TYPE_POWER, NVML_FI_DEV_POWER_INSTANT, 0 });
    }

    uint32_t deviceCount = 0;

    if(nvmlDeviceGetCount(&deviceCount) != NVML_SUCCESS)
    {
        return;
    }

    int suffixLen = log10(deviceCount) + 1;

    for(uint32_t i = 0; i < deviceCount; ++i)
    {
        nvmlDevice_t deviceHandle;
        if(nvmlDeviceGetHandleByIndex(i, &deviceHandle) != NVML_SUCCESS)
            continue;

        auto deviceDescRes = s_deviceDesc.insert(std::make_pair(i, DeviceDesc_t{ deviceHandle }));
        DeviceDesc_t &deviceDesc = deviceDescRes.first->second;

        for(const SourceDecl_t &sourceDecl: sourceDecls)
        {
            const std::string fullName  = sysfsutils::MakeCounterName(sourceDecl.name, "", "", { { i, suffixLen } });
            const std::string fullLabel = sysfsutils::MakeCounterName(sourceDecl.name, "", "", { { i, suffixLen } });

            SourceDesc_t sourceDesc(sourceDecl, fullLabel, fullName, &deviceDesc);

            auto baseRes = s_sourceDesc.insert(std::make_pair(sourceDecl.name, std::map<int, SourceDesc_t>()));
            auto res     = baseRes.first->second.insert(std::make_pair(i, std::move(sourceDesc)));

            s_sourceByName[fullName] = &res.first->second;

            if((sourceDecl.type & SourceDecl_t::TYPE_POWER) != 0)
            {
                deviceDesc.fieldValues.push_back(nvmlFieldValue_t{});
                deviceDesc.fieldValues.back().fieldId = sourceDecl.fieldId;
                deviceDesc.fieldValues.back().scopeId = sourceDecl.scopeId;
                deviceDesc.sources.push_back(&res.first->second);
            }
        }
    }
}

double NVML::getResultValue(CounterValue_t startValue, CounterValue_t stopValue,
    double runtime, int &cumulative, std::string counter, std::string regionName)
{
    cumulative = 1;
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
    if(it != s_sourceByName.end())
    {
        return it->second->label;
    }

    return counter;
}

double NVML::getUnit(std::string counterName)
{
    auto it = s_sourceByName.find(counterName);
    if(it != s_sourceByName.end())
    {
        return it->second->unit;
    }

    return -1.0;
}

void NVML::start(timespec &time, double sampleFreqHz)
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

void NVML::stop(timespec &time)
{
    s_sampleStoreEnabled.store(false);
    s_asyncSampler.SetPeriod(s_overflowCheckPeriod);
}

void NVML::initSamplers(bool detailed)
{
    for(auto &desc: s_sourceByName)
    {
        if(desc.second->type != SourceDecl_t::TYPE_POWER)
        {
            continue;
        }

        s_sourceById.push_back(desc.second);
    }

    std::sort(s_sourceById.begin(), s_sourceById.end(),
        [](const SourceDesc_t *pLeft, const SourceDesc_t *pRight) {
            return pLeft->fullName < pRight->fullName;
        });
    
    // NOTR: Simulates HDEEM Blade sensor, which doesn't exist in NVML
    s_sourceById.insert(s_sourceById.begin(), nullptr);

    for(SourceDesc_t *pSourceDesc: s_sourceById)
    {
        vrSensorLabel.push_back(pSourceDesc ? pSourceDesc->fullName : "");
    }

    PROBE_NUMBER = s_sourceById.size();

    s_asyncSampler.Start(s_overflowCheckPeriod, [](bool isSampling, CounterRecord_t *pRequest) {
        for(auto &deviceDescIt: s_deviceDesc)
        {
            timespec now;
            clock_gettime(CLOCK_REALTIME, &now);

            auto &fieldValues = deviceDescIt.second.fieldValues;
            auto &sources     = deviceDescIt.second.sources;

            nvmlReturn_t ret = nvmlDeviceGetFieldValues(deviceDescIt.second.device,
                fieldValues.size(), fieldValues.data());
            
            for(size_t i = 0; i < fieldValues.size(); ++i)
            {
                uint64_t value = 0;
                if(ret == NVML_SUCCESS && fieldValues[i].nvmlReturn == NVML_SUCCESS)
                {
                    value = fieldValues[i].value.uiVal;

                    if(value < sources[i]->lastValue && sources[i]->type == SourceDecl_t::TYPE_ENERGY)
                        sources[i]->overflowCount++;

                    sources[i]->lastValue = value;
                }

                if(isSampling && s_sampleStoreEnabled)
                {
                    sources[i]->samples.push_back({
                        uint64_t(now.tv_sec) * 1000000000ull + now.tv_nsec,
                        value
                    });
                }
            }
        }

        if(!pRequest)
            return;

        for(auto &desc: s_sourceByName)
        {
            (*pRequest)[desc.second->fullName] = CounterValue_t(desc.second->lastValue);
        }
    });
}

void NVML::closeSamplers()
{
    s_asyncSampler.Stop();
}

int NVML::getSamples()
{
    return 0;
}

void NVML::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> &samples)
{
    if(sensorId >= s_sourceById.size() || !s_sourceById[sensorId])
    {
        return;
    }

    const SourceDesc_t &desc = *s_sourceById[sensorId];
    samples.reserve(last - first);
    for(size_t i = first; i < last; ++i)
    {
        if((desc.type & SourceDecl_t::TYPE_POWER) != 0)
        {
            samples.push_back(double(desc.samples[i].value) * desc.unit);
        }
    }
}

std::string NVML::getData(double startTime, double duration, 
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
            double power = (double(it->value) * desc.unit);

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

bool NVML::isAvailable()
{
    return true;
}

nvmlDevice_t NVML::getGPUHandle(int id)
{
    auto it = s_deviceDesc.find(id);
    if(it == s_deviceDesc.end())
        return nvmlDevice_t{};
    return it->second.device;
}

size_t NVML::getGPUsPerNode()
{
    return s_deviceDesc.size();
}

#else

struct NVML::SourceDecl_t {
};

struct NVML::SourceDesc_t {
};

struct NVML::DeviceDesc_t {
};

const std::string NVML::SUM_ENERGY_LABEL = "Energy consumption [J]";
int NVML::PROBE_NUMBER;
std::vector<std::string> NVML::vrSensorLabel;

NVML::SourceDescMap_t NVML::s_sourceDesc;
NVML::SourceNameMap_t NVML::s_sourceByName;
NVML::SourceIdVec_t   NVML::s_sourceById;
NVML::DeviceDescMap_t NVML::s_deviceDesc;

std::chrono::nanoseconds              NVML::s_overflowCheckPeriod(std::chrono::seconds(10));
std::atomic<bool>                      NVML::s_sampleStoreEnabled(false);
AsyncSampler<NVML::CounterRecord_t> NVML::s_asyncSampler;

void NVML::init(bool detailed)
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

double NVML::getResultValue(CounterValue_t startValue, CounterValue_t stopValue,
    double runtime, int &cumulative, std::string counter, std::string regionName)
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

void NVML::start(timespec &time, double sampleFreqHz)
{
}

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

std::string NVML::getData(double startTime, double duration, 
    size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR,
    double energy[], double minWatts[], double maxWatts[])
{
    return "";
}

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
