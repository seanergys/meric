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

#if   defined HAVE_MSR_SAFE
    namespace MSR_RAPL_COUNTERS_NS = MSR_RAPL_COUNTERS;
#elif defined HAVE_X86_ADAPT_H
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

const std::string RAPL::SUM_ENERGY_LABEL = "Energy consumption [J]";
int RAPL::PROBE_NUMBER;
std::vector<std::string> RAPL::vrSensorLabel;

std::map<std::string, RAPL::SamplerDesc_t> RAPL::s_samplerDesc;
std::vector<RAPL::SamplerDesc_t *>         RAPL::s_samplerById;

std::chrono::nanoseconds RAPL::s_samplingPeriod(std::chrono::milliseconds(25));
std::chrono::nanoseconds RAPL::s_overflowCheckPeriod(std::chrono::seconds(10));

std::atomic<bool>                            RAPL::s_sampleStoreEnabled(false);
AsyncSampler<RAPL::Base::CounterRecord_t> RAPL::s_asyncSampler;

template<typename T>
double SampleEnergyToPower(const T &sample, uint64_t rangeMax, const T *pPrevSample) {
    if(!pPrevSample)
        return 0.0;

    uint64_t value = (sample.value < pPrevSample->value) ? ((rangeMax - pPrevSample->value) + sample.value) : (sample.value - pPrevSample->value);
    double dt = 1.0f / (double(sample.timestamp - pPrevSample->timestamp) * 1.0e-9);

    return double(value) * dt;
};

void RAPL::init(bool detailed)
{
    getImpl()->init(detailed);
    initSamplers();
}

void RAPL::close()
{
    closeSamplers();
    getImpl()->close();
}

void RAPL::getCounterValues(CounterRecord_t &record)
{
    // getImpl()->getCounterValues(record);
    s_asyncSampler.Sample(&record);
}

double RAPL::getResultValue(CounterValue_t startValue, CounterValue_t stopValue, 
    double runtime, int &cumulative, std::string counter, std::string regionName)
{
    return getImpl()->getResultValue(startValue, stopValue, runtime, cumulative, counter);
}

std::string RAPL::summaryCounter()
{
    return getImpl()->summaryCounter();
}

std::string RAPL::getSummaryCounterName()
{
    return getImpl()->getSummaryCounterName();
}

std::string RAPL::getSourceLabel()
{
    return getImpl()->getSourceLabel();
}

std::string RAPL::getCounterLabel(const std::string counter)
{
    return getImpl()->getCounterLabel(counter);
}

void RAPL::getRatios(CounterRecord_t &start, CounterRecord_t &stop,
    double runtime, std::map<std::string, double> &ratios)
{
    getImpl()->getRatios(start, stop, runtime, ratios);
}

void RAPL::initCountersUnits()
{
    getImpl()->initCountersUnits();
}

double RAPL::getUnit(std::string counter)
{
    return getImpl()->getUnit(counter);
}

void RAPL::start(timespec &time, double sampleFreqHz)
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

void RAPL::stop(timespec &time)
{
    s_sampleStoreEnabled.store(false);
    s_asyncSampler.SetPeriod(s_overflowCheckPeriod);
}

void RAPL::initSamplers()
{
    {
        std::vector<Base::CounterDesc_t *> counters;
        getImpl()->getCounters(counters);

        std::sort(counters.begin(), counters.end(),
            [](const Base::CounterDesc_t *pLeft, const Base::CounterDesc_t *pRight) {
                if(!pLeft)
                {
                    return true;
                }

                if(!pRight)
                {
                    return false;
                }

                return pLeft->fullName < pRight->fullName;
            });
        
        // NOTE: Simulates HDEEM Blade sensor, which doesn't exist in RAPL
        vrSensorLabel.push_back("");
        s_samplerById.push_back(nullptr);
        
        for(Base::CounterDesc_t *pCounterDesc: counters)
        {
            vrSensorLabel.push_back(pCounterDesc->fullName);

            auto res = s_samplerDesc.insert(std::make_pair(pCounterDesc->fullName, SamplerDesc_t()));
            res.first->second.pCounterDesc  = pCounterDesc;
            res.first->second.lastValue     = 0;
            res.first->second.overflowCount = 0;

            s_samplerById.push_back(&res.first->second);
        }

        PROBE_NUMBER = s_samplerById.size();
    }

    s_asyncSampler.Start(s_overflowCheckPeriod, [](bool isSampling, CounterRecord_t *pRequest) {
        for(auto &desc: s_samplerDesc)
        {
            timespec now;
            clock_gettime(CLOCK_REALTIME, &now);

            uint64_t value = getImpl()->getCounterValue(desc.second.pCounterDesc);

            if(isSampling && s_sampleStoreEnabled)
            {
                desc.second.samples.push_back({
                    mutils::timeToNanoSec(now),
                    value
                });
            }

            if(value < desc.second.lastValue)
                desc.second.overflowCount++;

            desc.second.lastValue = value;
        }

        if(!pRequest)
            return;

        for(auto &desc: s_samplerDesc)
        {
            (*pRequest)[desc.first] = CounterValue_t(desc.second.lastValue, desc.second.overflowCount);
        }
    });
}

void RAPL::closeSamplers()
{
    s_asyncSampler.Stop();
}

int RAPL::getSamples()
{
    return 0;
}

void RAPL::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> &samples)
{
    if(sensorId >= s_samplerById.size() || !s_samplerById[sensorId])
    {
        return;
    }

    const SamplerDesc_t &desc = *s_samplerById[sensorId];
    samples.reserve(last - first);
    for(size_t i = first; i < last; ++i)
    {
        samples.push_back(SampleEnergyToPower(desc.samples[i], desc.pCounterDesc->rangeMax, 
            i > 0 ? &desc.samples[i - 1] : nullptr) * desc.pCounterDesc->unit);
    }
}

std::string RAPL::getData(double startTime, double duration,
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
        auto endIt = std::upper_bound(desc.samples.begin(), desc.samples.end(), intervalEnd,
            [](uint64_t value, const Sample_t &sample) { return value < sample.timestamp; });
        
        energy[i]   = 0.0;
        minWatts[i] = std::numeric_limits<double>::max();
        maxWatts[i] = 0.0;

        for(auto it = startIt; it != endIt; ++it)
        {
            // double power = double(it->value) * desc.pCounterDesc->unit;
            double power = SampleEnergyToPower(*it, desc.pCounterDesc->rangeMax, 
                it != desc.samples.begin() ? (it - 1).base() : nullptr) * desc.pCounterDesc->unit;
            energy[i] += power;
            minWatts[i] = std::min(minWatts[i], power);
            maxWatts[i] = std::max(maxWatts[i], power);
        }

        *firstVR = std::distance(desc.samples.begin(), startIt);
        *lastVR  = std::distance(desc.samples.begin(), endIt);
    }

    *firstBlade = *lastBlade = 0;
    return "";
}

class RAPL_NONE : public RAPL::Base
{
public:
    virtual void init(bool detailed) {}
    virtual void close() {}

    virtual void getCounterValues(CounterRecord_t &record) {}
    virtual double getResultValue(CounterValue_t startValue, CounterValue_t stopValue, 
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

#if defined(HAVE_MSR_SAFE) || defined(HAVE_MSR_CORE_H)

class RAPL_MSR : public RAPL::Base {
public:
    virtual void init(bool detailed);
    virtual void close();

    virtual void getCounterValues(CounterRecord_t &record);
    virtual double getResultValue(CounterValue_t start, CounterValue_t stop,
        double runtime, int &cumulative, std::string counter, std::string regionName = "");
    
    virtual std::string summaryCounter();
    virtual std::string getSummaryCounterName();
    virtual std::string getSourceLabel();
    virtual std::string getCounterLabel(const std::string counter);

    virtual void getRatios(CounterRecord_t &start, CounterRecord_t &stop,
        double runtime, std::map<std::string, double> &ratios) {}
    
    virtual void initCountersUnits();
    virtual double getUnit(std::string counter);

    virtual void getCounters(std::vector<Base::CounterDesc_t *> &counters);
    virtual RawCounterValue_t getCounterValue(Base::CounterDesc_t *pCounterDesc);

    static bool isAvailable();

protected:
    struct CounterDesc_t : public Base::CounterDesc_t
    {
        int cpuid;
        off_t reg_address;
    };

    double getCounterUnit(const std::string &name);

    bool detailedMode;
    std::map<std::string, CounterDesc_t> counterDesc;

    // FIXME: This shouldn't be part of RAPL_MSR as its used for power control knobs,
    //        not the counters!
    std::map<std::string, double> units;
};

void RAPL_MSR::init(bool detailed)
{
    using namespace MSR_RAPL_COUNTERS_NS;

    struct CounterDef_t {
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

        std::string name;
        Domain_t domain;
        uint32_t flags;
        int count;
    };

    auto GetCoreID = [](CounterDef_t::Domain_t domain, int counterIdx) {
        switch(domain)
        {
        case CounterDef_t::DOMAIN_PER_SOCKET:
            return Environment::mapCoresToSocket[counterIdx][0];
            break;
        case CounterDef_t::DOMAIN_PER_CORE:
            return Environment::mapThreadsToCore[counterIdx][0];
            break;
        default:
            return counterIdx;
        }
    };

    detailedMode = detailed;
    std::vector<CounterDef_t> counterDefs;

#ifdef _INTEL_
    counterDefs.push_back({ RAPL_PCKG_ENERGY, CounterDef_t::DOMAIN_PER_SOCKET, CounterDef_t::FLAG_ACCUMULATIVE_BIT, Environment::SOCKETS_PER_NODE });
    counterDefs.push_back({ RAPL_RAM_ENERGY, CounterDef_t::DOMAIN_PER_SOCKET, CounterDef_t::FLAG_ACCUMULATIVE_BIT, Environment::SOCKETS_PER_NODE });
#elif defined(_AMD_)
    counterDefs.push_back({ RAPL_PCKG_ENERGY, CounterDef_t::DOMAIN_PER_SOCKET, CounterDef_t::FLAG_ACCUMULATIVE_BIT, Environment::SOCKETS_PER_NODE });

    if(detailed)
    {
        counterDefs.push_back({ RAPL_CORE_ENERGY, CounterDef_t::DOMAIN_PER_CORE, CounterDef_t::FLAG_NONE_BIT, Environment::SOCKETS_PER_NODE * Environment::CORES_PER_SOCKET });
    }
#endif // Platform

    for(const auto &counterDef: counterDefs)
    {
        const int suffixLen       = log10(counterDef.count) + 1;
        const double counterUnit  = getCounterUnit(counterDef.name);
        const uint64_t counterMax = uint64_t(1) << 32;

        for(int i = 0; i < counterDef.count; ++i)
        {
            const std::string fullName  = sysfsutils::MakeCounterName(counterDef.name, counterLabel, {{ i, suffixLen }});
            const std::string fullLabel = sysfsutils::MakeCounterName(counterDef.name, counterLabel, {{ i, suffixLen }}, true);

            auto res = counterDesc.insert(std::make_pair(fullName, CounterDesc_t()));
            CounterDesc_t &desc = res.first->second;
            desc.label        = fullLabel;
            desc.fullName     = fullName;
            desc.rangeMax     = counterMax;
            desc.unit         = counterUnit;
            desc.isCumulative = (counterDef.flags & CounterDef_t::FLAG_ACCUMULATIVE_BIT) != 0;

            desc.cpuid       = GetCoreID(counterDef.domain, i);
            desc.reg_address = counterDefines[counterDef.name];
        }
    }
}

void RAPL_MSR::close()
{

}

void RAPL_MSR::getCounterValues(CounterRecord_t &record)
{
    for(const auto &desc: counterDesc)
    {
        uint64_t value = 0;
        read_msr(desc.second.cpuid, desc.second.reg_address, &value);
        record[desc.first] = CounterValue_t(value & MSR_ANYDOMAIN_ENERGY_STATUS_mask, 0);
    }
}

double RAPL_MSR::getResultValue(CounterValue_t start, CounterValue_t stop,
    double runtime, int &cumulative, std::string counter, std::string regionName)
{
    auto it = counterDesc.find(counter);
    if(it == counterDesc.end())
    {
        return 0.0;
    }

    cumulative = it->second.isCumulative ? 1 : 0;
    uint64_t overflow = stop.generation - start.generation;
    double res = double(stop.value) - double(start.value);

    return (res + overflow * double(it->second.rangeMax)) * it->second.unit;
}

std::string RAPL_MSR::summaryCounter()
{
    using namespace MSR_RAPL_COUNTERS_NS;

    return RAPL_SUM_ENERGY;
}

std::string RAPL_MSR::getSummaryCounterName()
{
    using namespace MSR_RAPL_COUNTERS_NS;

    auto it = counterLabel.find(summaryCounter());
    if(it != counterLabel.end())
    {
        return it->second.first + " " + it->second.second;
    }

    return summaryCounter();
}

std::string RAPL_MSR::getSourceLabel()
{
    return "RAPL";
}

std::string RAPL_MSR::getCounterLabel(const std::string counter)
{
    using namespace MSR_RAPL_COUNTERS_NS;

    auto it = counterDesc.find(counter);
    if(it != counterDesc.end())
    {
        return it->second.label;
    }

    return counter;
}

void RAPL_MSR::initCountersUnits()
{
    using namespace MSR_RAPL_COUNTERS_NS;

    if(!units.empty())
        return; // Don't reinitialize units

    RAPL::PCKG_POWER_LIMIT       = RAPL_PCKG_POWER_LIMIT;
    RAPL::RAM_POWER_LIMIT        = RAPL_RAM_POWER_LIMIT;
    RAPL::PLATFORM_POWER_LIMIT   = RAPL_PLATFORM_POWER_LIMIT;
    RAPL::POWER_LIMIT_TIMEWINDOW = RAPL_POWER_LIMIT_TIMEWINDOW;

    uint64_t MSR_RAPL_POWER_UNIT_bits = 0;
    read_msr(0, MSR_RAPL_POWER_UNIT, &MSR_RAPL_POWER_UNIT_bits);

    if(MSR_RAPL_POWER_UNIT_bits == 0)
        return;

    units[RAPL_PCKG_POWER_LIMIT]     = 1.0 / pow(2, (MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_power_mask));
    units[RAPL_RAM_POWER_LIMIT]      = units[RAPL_PCKG_POWER_LIMIT];
    units[RAPL_PLATFORM_POWER_LIMIT] = units[RAPL_PCKG_POWER_LIMIT];

    units[RAPL_POWER_LIMIT_TIMEWINDOW]   = 1.0 / pow(2, ((MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_time_mask) >> MSR_RAPL_POWER_UNIT_time_shift));
    units[MSR_COUNTERS::RAPL_PKG_ACTIVE] = units[RAPL_POWER_LIMIT_TIMEWINDOW];
}

double RAPL_MSR::getUnit(std::string counter)
{
    // Counter units are assumed to be initialized with RAPL::init(...) call.
    {
        auto it = counterDesc.find(counter);
        if(it != counterDesc.end())
        {
            return it->second.unit;
        }
    }

    // Power control knob units are initialized by RAPL::initCounterUnits() call.
    {
        auto it = units.find(counter);
        if(it != units.end())
        {
            return it->second;
        }
    }

    return -1.0;
}

void RAPL_MSR::getCounters(std::vector<Base::CounterDesc_t *> &counters)
{
    counters.clear();
    for(auto &item: counterDesc)
    {
        counters.push_back(&item.second);
    }
}

RAPL_MSR::RawCounterValue_t RAPL_MSR::getCounterValue(Base::CounterDesc_t *pCounterDesc)
{
    CounterDesc_t *pDesc = static_cast<CounterDesc_t *>(pCounterDesc);

    uint64_t value = 0;
    if(!pDesc || read_msr(pDesc->cpuid, pDesc->reg_address, &value) != 0)
        return 0;

    return (value & MSR_ANYDOMAIN_ENERGY_STATUS_mask);
}

bool RAPL_MSR::isAvailable()
{
    using namespace MSR_RAPL_COUNTERS_NS;

    return check_whitelist_msr({});
}

double RAPL_MSR::getCounterUnit(const std::string &name)
{
    using namespace MSR_RAPL_COUNTERS_NS;

#ifdef _INTEL_
    if(name == RAPL_RAM_ENERGY)
    {
        return 1.0 / pow(2, 16);
    }
#endif // _INTEL_

    if(counterLabel.count(name) > 0)
    {
        uint64_t MSR_RAPL_POWER_UNIT_bits = 0;
        read_msr(0, MSR_RAPL_POWER_UNIT, &MSR_RAPL_POWER_UNIT_bits);

        if(MSR_RAPL_POWER_UNIT_bits != 0)
            return 1.0 / pow(2, ((MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_energy_mask) >> MSR_RAPL_POWER_UNIT_energy_shift));
    }

    return -1.0;
}

#endif // RAPL_MSR

#if defined(HAVE_GEOPM_H)

class RAPL_GEOPM : public RAPL::Base {
public:
    virtual void init(bool detailed);
    virtual void close();

    virtual void getCounterValues(CounterRecord_t &record);
    virtual double getResultValue(CounterValue_t start, CounterValue_t stop,
        double runtime, int &cumulative, std::string counter, std::string regionName = "");
    
    virtual std::string summaryCounter();
    virtual std::string getSummaryCounterName();
    virtual std::string getSourceLabel();
    virtual std::string getCounterLabel(const std::string counter);

    virtual void getRatios(CounterRecord_t &start, CounterRecord_t &stop,
        double runtime, std::map<std::string, double> &ratios) {}
    
    virtual void initCountersUnits();
    virtual double getUnit(std::string counter);

    virtual void getCounters(std::vector<Base::CounterDesc_t *> &counters);
    virtual RawCounterValue_t getCounterValue(Base::CounterDesc_t *pCounterDesc);

    static bool isAvailable();

protected:
    struct CounterDesc_t : public Base::CounterDesc_t
    {
        int cpuid;
        std::string name;
    };

    double getCounterUnit(const std::string &name);

    bool detailedMode;
    std::map<std::string, CounterDesc_t> counterDesc;

    // FIXME: This shouldn't be part of RAPL_MSR as its used for power control knobs,
    //        not the counters!
    std::map<std::string, double> units;
};

void RAPL_GEOPM::init(bool detailed)
{
    using namespace GEOPM_RAPL_COUNTERS;

    struct CounterDef_t {
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

        std::string name;
        Domain_t domain;
        uint32_t flags;
        int count;
    };

    auto GetCoreID = [](CounterDef_t::Domain_t domain, int counterIdx) {
        switch(domain)
        {
        case CounterDef_t::DOMAIN_PER_SOCKET:
            return Environment::mapThreadsToCore[counterIdx][0];
            break;
        case CounterDef_t::DOMAIN_PER_CORE:
            return Environment::mapCoresToSocket[counterIdx][0];
            break;
        default:
            return counterIdx;
        }
    };

    detailedMode = detailed;
    std::vector<CounterDef_t> counterDefs;

#ifdef _INTEL_
    counterDefs.push_back({ RAPL_PCKG_ENERGY, CounterDef_t::DOMAIN_PER_SOCKET, CounterDef_t::FLAG_ACCUMULATIVE_BIT, Environment::SOCKETS_PER_NODE });
    counterDefs.push_back({ RAPL_RAM_ENERGY, CounterDef_t::DOMAIN_PER_SOCKET, CounterDef_t::FLAG_ACCUMULATIVE_BIT, Environment::SOCKETS_PER_NODE });
#elif defined(_AMD_)
    counterDefs.push_back({ RAPL_PCKG_ENERGY, CounterDef_t::DOMAIN_PER_SOCKET, CounterDef_t::FLAG_ACCUMULATIVE_BIT, Environment::SOCKETS_PER_NODE });

    if(detailed)
    {
        counterDefs.push_back({ RAPL_CORE_ENERGY, CounterDef_t::DOMAIN_PER_CORE, CounterDef_t::FLAG_NONE_BIT, Environment::SOCKETS_PER_NODE * Environment::CORES_PER_SOCKET });
    }
#endif // Platform

    for(const auto &counterDef: counterDefs)
    {
        const int suffixLen       = log10(counterDef.count) + 1;
        const double counterUnit  = getCounterUnit(counterDef.name);
        const uint64_t counterMax = uint64_t(1) << 32;

        for(int i = 0; i < counterDef.count; ++i)
        {
            const std::string fullName  = sysfsutils::MakeCounterName(counterDef.name, counterLabel, {{ i, suffixLen }});
            const std::string fullLabel = sysfsutils::MakeCounterName(counterDef.name, counterLabel, {{ i, suffixLen }}, true);

            auto res = counterDesc.insert(std::make_pair(fullName, CounterDesc_t()));
            CounterDesc_t &desc = res.first->second;
            desc.label        = fullLabel;
            desc.fullName     = fullName;
            desc.rangeMax     = counterMax;
            desc.unit         = counterUnit;
            desc.isCumulative = (counterDef.flags & CounterDef_t::FLAG_ACCUMULATIVE_BIT) != 0;

            desc.cpuid = GetCoreID(counterDef.domain, i);
            desc.name  = counterDef.name;
        }
    }
}

void RAPL_GEOPM::close()
{

}

void RAPL_GEOPM::getCounterValues(CounterRecord_t &record)
{
    for(const auto &desc: counterDesc)
    {
        uint64_t value = geopm_signal_to_field(pio.read_signal(desc.second.name, 
            pio.signal_domain_type(desc.second.name), desc.second.cpuid));
        record[desc.first] = CounterValue_t(value & MSR_ANYDOMAIN_ENERGY_STATUS_mask);
    }
}

double RAPL_GEOPM::getResultValue(CounterValue_t start, CounterValue_t stop,
    double runtime, int &cumulative, std::string counter, std::string regionName)
{
    auto it = counterDesc.find(counter);
    if(it == counterDesc.end())
    {
        return 0.0;
    }

    cumulative = it->second.isCumulative ? 1 : 0;
    uint64_t overflow = stop.generation - start.generation;
    double res = double(stop.value) - double(start.value);

    return (res + overflow * double(it->second.rangeMax)) * it->second.unit;
}

std::string RAPL_GEOPM::summaryCounter()
{
    using namespace GEOPM_RAPL_COUNTERS;

    return RAPL_SUM_ENERGY;
}

std::string RAPL_GEOPM::getSummaryCounterName()
{
    using namespace GEOPM_RAPL_COUNTERS;

    auto it = counterLabel.find(summaryCounter());
    if(it != counterLabel.end())
    {
        return it->second.first + " " + it->second.second;
    }

    return summaryCounter();
}

std::string RAPL_GEOPM::getSourceLabel()
{
    return "RAPL";
}

std::string RAPL_GEOPM::getCounterLabel(const std::string counter)
{
    using namespace GEOPM_RAPL_COUNTERS;

    auto it = counterDesc.find(counter);
    if(it != counterDesc.end())
    {
        return it->second.label;
    }

    return counter;
}

void RAPL_GEOPM::initCountersUnits()
{
    using namespace GEOPM_RAPL_COUNTERS;

    if(!units.empty())
        return; // Don't reinitialize units

    RAPL::PCKG_POWER_LIMIT       = RAPL_PCKG_POWER_LIMIT;
    RAPL::RAM_POWER_LIMIT        = RAPL_RAM_POWER_LIMIT;
    RAPL::PLATFORM_POWER_LIMIT   = RAPL_PLATFORM_POWER_LIMIT;
    RAPL::POWER_LIMIT_TIMEWINDOW = RAPL_POWER_LIMIT_TIMEWINDOW;

    uint64_t MSR_RAPL_POWER_UNIT_bits = 0;
    MSR_RAPL_POWER_UNIT_bits = geopm_signal_to_field(pio.read_signal("MSR::RAPL_POWER_UNIT#", pio.signal_domain_type("MSR::RAPL_POWER_UNIT#"), 0));

    if(MSR_RAPL_POWER_UNIT_bits == 0)
        return;

    units[RAPL_PCKG_POWER_LIMIT]     = 1.0 / pow(2, (MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_power_mask));
    units[RAPL_RAM_POWER_LIMIT]      = units[RAPL_PCKG_POWER_LIMIT];
    units[RAPL_PLATFORM_POWER_LIMIT] = units[RAPL_PCKG_POWER_LIMIT];

    units[RAPL_POWER_LIMIT_TIMEWINDOW]   = 1.0 / pow(2, ((MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_time_mask) >> MSR_RAPL_POWER_UNIT_time_shift));
    units[MSR_COUNTERS::RAPL_PKG_ACTIVE] = units[RAPL_POWER_LIMIT_TIMEWINDOW];
}

double RAPL_GEOPM::getUnit(std::string counter)
{
    // Counter units are assumed to be initialized with RAPL::init(...) call.
    {
        auto it = counterDesc.find(counter);
        if(it != counterDesc.end())
        {
            return it->second.unit;
        }
    }

    // Power control knob units are initialized by RAPL::initCounterUnits() call.
    {
        auto it = units.find(counter);
        if(it != units.end())
        {
            return it->second;
        }
    }

    return -1.0;
}

void RAPL_GEOPM::getCounters(std::vector<Base::CounterDesc_t *> &counters)
{
    counters.clear();
    for(auto &item: counterDesc)
    {
        counters.push_back(&item.second);
    }
}

RAPL_GEOPM::RawCounterValue_t RAPL_GEOPM::getCounterValue(Base::CounterDesc_t *pCounterDesc)
{
    CounterDesc_t *pDesc = static_cast<CounterDesc_t *>(pCounterDesc);

    uint64_t value = 0;
    if(!pDesc)
        return 0;

    value = geopm_signal_to_field(pio.read_signal(desc.second.name, 
        pio.signal_domain_type(desc.second.name), desc.second.cpuid));

    return (value & MSR_ANYDOMAIN_ENERGY_STATUS_mask);
}

bool RAPL_GEOPM::isAvailable()
{
    return true;
}

double RAPL_GEOPM::getCounterUnit(const std::string &name)
{
    using namespace GEOPM_RAPL_COUNTERS;

    if(name == RAPL_RAM_ENERGY)
    {
        return 1.0 / pow(2, 16);
    }
    else if(counterLabel.count(name) > 0)
    {
        uint64_t MSR_RAPL_POWER_UNIT_bits = 0;
        MSR_RAPL_POWER_UNIT_bits = geopm_signal_to_field(pio.read_signal("MSR::RAPL_POWER_UNIT#", pio.signal_domain_type("MSR::RAPL_POWER_UNIT#"), 0));

        if(MSR_RAPL_POWER_UNIT_bits != 0)
            return 1.0 / pow(2, ((MSR_RAPL_POWER_UNIT_bits & MSR_RAPL_POWER_UNIT_energy_mask) >> MSR_RAPL_POWER_UNIT_energy_shift));
    }

    return -1.0;
}

#endif // RAPL_GEOPM

class RAPL_SYSFS : public RAPL::Base {
public:
    virtual void init(bool detailed);
    virtual void close();

    virtual void getCounterValues(CounterRecord_t &record);
    virtual double getResultValue(CounterValue_t startValue, CounterValue_t stopValue, 
        double runtime, int &cumulative, std::string counter, std::string regionName = "");

    virtual std::string summaryCounter();
    virtual std::string getSummaryCounterName();
    virtual std::string getSourceLabel();
    virtual std::string getCounterLabel(const std::string counter);

    virtual void getRatios(CounterRecord_t &start, CounterRecord_t &stop,
        double runtime, std::map<std::string, double> &ratios) {}

    virtual void initCountersUnits() {}
    virtual double getUnit(std::string counter);

    virtual void getCounters(std::vector<Base::CounterDesc_t *> &counters);
    virtual RawCounterValue_t getCounterValue(CounterDesc_t *pCounterDesc);

    static bool isAvailable();

protected:
    struct CounterDesc_t : public Base::CounterDesc_t
    {
        std::string path;
        std::ifstream fs;
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

    if(detailed)
    {
        names.push_back(RAPL_PCKG_CORES_ENERGY);
    }
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

            if(!sysfsutils::ReadValue(dieRootPath + pszDomainName + "/name", sysfsName))
            {
                std::cerr << "MERIC WARNING: Failed to open: " << (dieRootPath + pszDomainName + "/name") << std::endl;
                continue;
            }

            if(!sysfsutils::ReadValue(dieRootPath + pszDomainName + "/max_energy_range_uj", maxCounterRange))
            {
                std::cerr << "MERIC WARNING: Failed to open: " << (dieRootPath + pszDomainName + "/max_energy_range_uj") << std::endl;
                continue;
            }

            if(access((dieRootPath + pszDomainName + "/energy_uj").c_str(), R_OK) != 0)
            {
                std::cerr << "MERIC WARNING: Failed to open: " << (dieRootPath + pszDomainName + "/energy_uj") << std::endl;
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
            
            auto res = counterDesc.insert(std::make_pair(fullName, CounterDesc_t()));
            res.first->second.label    = fullLabel;
            res.first->second.fullName = fullName;
            res.first->second.rangeMax = maxCounterRange;
            res.first->second.unit     = 1.0e-6;
            res.first->second.path     = dieRootPath + pszDomainName + "/energy_uj";
            res.first->second.fs       = std::ifstream(dieRootPath + pszDomainName + "/energy_uj");

            #if defined(_INTEL_)
                res.first->second.isCumulative = true;
            #elif defined(_AMD_)
                res.first->second.isCumulative = (counterBaseName == RAPL_PCKG_ENERGY);
            #else
                res.first->second.isCumulative = false;
            #endif
        }
    }
}

void RAPL_SYSFS::close()
{
    for(auto it = counterDesc.begin(); it != counterDesc.end(); ++it)
    {
        it->second.fs.close();
    }
    counterDesc.clear();
}

void RAPL_SYSFS::getCounterValues(CounterRecord_t &record)
{
    for(const auto &desc: counterDesc)
    {
        record[desc.first] = CounterValue_t(0, 0);
    }

    for(auto &desc: counterDesc)
    {
        uint64_t value = 0;
        sysfsutils::ReadValue(desc.second.fs, value);
        record[desc.first] = CounterValue_t(value, 0);
    }
}

double RAPL_SYSFS::getResultValue(CounterValue_t startValue, CounterValue_t stopValue, 
    double runtime, int &cumulative, std::string counter, std::string regionName)
{
    auto it = counterDesc.find(counter);
    if(it == counterDesc.end())
    {
        return 0.0;
    }

    cumulative = it->second.isCumulative ? 1 : 0;
    uint64_t overflow = stopValue.generation - startValue.generation;
    double res = double(stopValue.value) - double(startValue.value);

    return (res + overflow * double(it->second.rangeMax)) * it->second.unit;
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

void RAPL_SYSFS::getCounters(std::vector<Base::CounterDesc_t *> &counters)
{
    counters.clear();
    for(auto &item: counterDesc)
    {
        counters.push_back(&item.second);
    }
}

RAPL_SYSFS::RawCounterValue_t RAPL_SYSFS::getCounterValue(Base::CounterDesc_t *pCounterDesc)
{
    CounterDesc_t *pDesc = static_cast<CounterDesc_t *>(pCounterDesc);
    uint64_t value = 0;

    if(!pDesc || !sysfsutils::ReadValue(pDesc->fs, value))
        return 0;

    return value;
}

bool RAPL_SYSFS::isAvailable()
{
    if(access("/sys/class/powercap/intel-rapl:0/name", R_OK) != 0)
    {
        return false;
    }
    if(access("/sys/class/powercap/intel-rapl:0/energy_uj", R_OK) != 0)
    {
        return false;
    }
    return true;
}

class RAPL_PERF_EVENT : public RAPL::Base {
public:
    virtual void init(bool detailed);
    virtual void close();

    virtual void getCounterValues(CounterRecord_t &record);
    virtual double getResultValue(CounterValue_t startValue, CounterValue_t stopValue, 
        double runtime, int &cumulative, std::string counter, std::string regionName = "");

    virtual std::string summaryCounter();
    virtual std::string getSummaryCounterName();
    virtual std::string getSourceLabel();
    virtual std::string getCounterLabel(const std::string counter);

    virtual void getRatios(CounterRecord_t &start, CounterRecord_t &stop,
        double runtime, std::map<std::string, double> &ratios) {}

    virtual void initCountersUnits() {}
    virtual double getUnit(std::string counter);

    virtual void getCounters(std::vector<Base::CounterDesc_t *> &counters);
    virtual RawCounterValue_t getCounterValue(CounterDesc_t *pCounterDesc);

    static bool isAvailable();

protected:
    template<size_t S = 8>
    struct Values_t
    {
        struct value
        {
            uint64_t value;
            uint64_t id;
        };
        
        uint64_t count_members;
        uint64_t time_enabled;
        uint64_t time_running;
        
        std::array<value, S> values;
    };

    struct CounterDesc_t : public Base::CounterDesc_t
    {
        CounterDesc_t()
            : fd(-1)
        {}

        CounterDesc_t(const CounterDesc_t &other) = delete;

        CounterDesc_t(CounterDesc_t &&other)
            : Base::CounterDesc_t(other)
            , fd(other.fd)
        {
            other.fd = -1;
        }

        ~CounterDesc_t()
        {
            close();
        }

        bool open(uint32_t type, uint64_t config, int cpu)
        {
            perf_event_attr event_attrib;
            std::memset(&event_attrib, 0, sizeof(perf_event_attr));

            event_attrib.type        = type;
            event_attrib.size        = sizeof(perf_event_attr);
            event_attrib.config      = config;
            event_attrib.sample_type = PERF_SAMPLE_IDENTIFIER;
            event_attrib.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING | PERF_FORMAT_GROUP | PERF_FORMAT_ID;
            event_attrib.disabled    = 1;
            event_attrib.inherit     = 1;

            fd = syscall(__NR_perf_event_open, &event_attrib, pid_t(-1), cpu, int(-1), 0);
            return (fd >= 0);
        }

        void close()
        {
            if(fd < 0)
            {
                return;
            }

            disable();
            ::close(fd);
            fd = -1;
        }

        int reset()   { if(fd < 0) return -1; return ::ioctl(fd, PERF_EVENT_IOC_RESET, 0); }
        int enable()  { if(fd < 0) return -1; return ::ioctl(fd, PERF_EVENT_IOC_ENABLE, 0); }
        int disable() { if(fd < 0) return -1; return ::ioctl(fd, PERF_EVENT_IOC_DISABLE, 0); }

        bool read(Values_t<> &values)
        {
            if(fd < 0)
            {
                return false;
            }
            return ::read(fd, &values, sizeof(Values_t<>)) >= sizeof(Values_t<1>);
        }

        int fd;
    };

    std::map<std::string, CounterDesc_t> counterDesc;
};

void RAPL_PERF_EVENT::init(bool detailed)
{
    using namespace PERF_RAPL_COUNTERS;

    const int socketSuffixLength = std::log10(Environment::SOCKETS_PER_NODE)+1;
    const int coreSuffixLength   = std::log10(Environment::SOCKETS_PER_NODE*Environment::CORES_PER_SOCKET)+1;

    auto OpenCounter = [&](const CounterDef_t &def, int cpu, int socket, int core)
    {
        std::vector<std::pair<int, int> > numbers;
        if(socket >= 0)
        {
            numbers.push_back({ socket, socketSuffixLength });
        }

        if(core >= 0)
        {
            numbers.push_back({ core, coreSuffixLength });
        }

        const std::string fullName  = sysfsutils::MakeCounterName(def.label, "", "", numbers);
        const std::string fullLabel = sysfsutils::MakeCounterName(def.label, "", def.unit, numbers);

        auto res = counterDesc.insert(std::make_pair(fullName, CounterDesc_t()));
        if(!res.second)
        {
            return true;
        }

        if(!res.first->second.open(def.type, def.config, cpu))
        {
            std::cerr << "MERIC ERROR: Failed to open perf event" 
                      << join(def.name.begin(), def.name.end(), std::string("/")) 
                      << " for " << fullName << std::endl;
            return false;
        }

        res.first->second.unit         = 1.0 / double(uint64_t(1) << 32);
        res.first->second.label        = fullLabel;
        res.first->second.fullName     = fullName;
        res.first->second.rangeMax     = std::numeric_limits<uint64_t>::max();
        res.first->second.isCumulative = (def.flags & CounterDef_t::FLAG_ACCUMULATIVE_BIT) != 0;

        return true;
    };

    auto FindCounterByName = [&](CounterDef_t &def) {
        const std::string basePath = "/sys/bus/event_source/devices/";
        if(def.name.size() < 2)
        {
            return true;
        }
        if(!sysfsutils::ReadValue(basePath + "/" + def.name[0] + "/type", def.type))
        {
            return false;
        }
        std::string event;
        if(!sysfsutils::ReadValue(basePath + "/" + def.name[0] + "/events/" + def.name[1], event))
        {
            return false;
        }
        if(sscanf(event.c_str(), "event=%lx", &def.config) < 1)
        {
            return false;
        }
        return true;
    };

    for(const auto &counterDef: counterDefs)
    {
        auto def = counterDef.second;
        if(!FindCounterByName(def))
        {
            continue;
        }

        if(!detailed && ((counterDef.second.flags & CounterDef_t::FLAG_DETAILED_ONLY_BIT) != 0))
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
                    OpenCounter(def, i, i / Environment::CORES_PER_SOCKET, i % Environment::CORES_PER_SOCKET);
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
            std::cerr << "MERIC ERROR: Failed to read perf event" << std::endl;
            record[it->first] = 0;
            continue;
        }

        record[it->first] = CounterValue_t(values.values[0].value, 0);
    }
}

double RAPL_PERF_EVENT::getResultValue(CounterValue_t startValue, CounterValue_t stopValue, 
    double runtime, int &cumulative, std::string counter, std::string regionName)
{
    auto it = counterDesc.find(counter);
    if(it == counterDesc.end())
    {
        return 0.0;
    }

    cumulative = it->second.isCumulative ? 1 : 0;
    uint64_t overflow = stopValue.generation - startValue.generation;
    double res = double(stopValue.value) - double(startValue.value);

    return (res + overflow * double(it->second.rangeMax)) * it->second.unit;
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
    auto it = counterDesc.find(counter);
    if(it != counterDesc.end())
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

void RAPL_PERF_EVENT::getCounters(std::vector<Base::CounterDesc_t *> &counters)
{
    counters.clear();
    for(auto &item: counterDesc)
    {
        counters.push_back(&item.second);
    }
}

RAPL_PERF_EVENT::RawCounterValue_t RAPL_PERF_EVENT::getCounterValue(Base::CounterDesc_t *pCounterDesc)
{
    Values_t<> values;

    CounterDesc_t *pDesc = static_cast<CounterDesc_t *>(pCounterDesc);
    if(!pDesc || !pDesc->read(values))
    {
        return 0;
    }

    return values.values[0].value;
}

bool RAPL_PERF_EVENT::isAvailable()
{
    int value = 2;
    
    if(!sysfsutils::ReadValue("/proc/sys/kernel/perf_event_paranoid", value))
    {
        return false;
    }

    return (value <= 0);
}

bool RAPL::isAvailable()
{
#if defined(HAVE_MSR_SAFE) || defined(HAVE_MSR_CORE_H)
    if(RAPL_MSR::isAvailable())
    {
        return true;
    }
#endif

#if defined(HAVE_GEOPM_H)
    if(RAPL_GEOPM::isAvailable())
    {
        return true;
    }
#endif

    if(RAPL_PERF_EVENT::isAvailable())
    {
        return true;
    }

    if(RAPL_SYSFS::isAvailable())
    {
        return true;
    }
    
    return false;
}

RAPL::Base *RAPL::getImpl()
{
    auto InitImpl = []() 
    {
    #if defined(HAVE_MSR_SAFE) || defined(HAVE_MSR_CORE_H)
        if(RAPL_MSR::isAvailable())
        {
            MERIC_INFO << "MERIC INFO: Using RAPL_MSR for RAPL energy measurements." << std::endl;
            return std::unique_ptr<Base>(new RAPL_MSR());
        }
    #endif

    #if defined(HAVE_GEOPM_H)
        if(RAPL_GEOPM::isAvailable())
        {
            MERIC_INFO << "MERIC INFO: Using RAPL_GEOPM for RAPL energy measurements." << std::endl;
            return std::unique_ptr<Base>(new RAPL_GEOPM());
        }
    #endif

        if(RAPL_PERF_EVENT::isAvailable())
        {
            MERIC_INFO << "MERIC INFO: Using RAPL_PERF_EVENT for RAPL energy measurements." << std::endl;
            return std::unique_ptr<Base>(new RAPL_PERF_EVENT());
        }

        if(RAPL_SYSFS::isAvailable())
        {
            MERIC_INFO << "MERIC INFO: Using RAPL_SYSFS for RAPL energy measurements." << std::endl;
            return std::unique_ptr<Base>(new RAPL_SYSFS());
        }
        MERIC_WARNING << "MERIC WARNING: No RAPL measurement method is available. RAPL measurements will be disabled." << std::endl;
        return std::unique_ptr<Base>(new RAPL_NONE());
    };

    static std::unique_ptr<Base> pImpl = InitImpl();

    return pImpl.get();
}
