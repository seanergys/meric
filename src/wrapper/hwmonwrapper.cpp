#include "hwmonwrapper.h"
#include "environmentwrapper.h"

using namespace meric;

struct HWMON::SourceDecl_t {
    enum Domain_t {
        DOMAIN_NONE       = 0,
        DOMAIN_GLOBAL     = 1,
        DOMAIN_PER_SOCKET = 2,
        DOMAIN_PER_CORE   = 3,
        DOMAIN_PER_THREAD = 4,
    };

    enum Flags_t {
        FLAG_NONE_BIT          = 0,
        FLAG_ACCUMULATIVE_BIT  = 1 << 0,
        FLAG_DETAILED_ONLY_BIT = 1 << 1,
        FLAG_DERIVED_BIT       = 1 << 2,
    };

    enum Type_t {
        TYPE_NONE = 0,
        TYPE_ENERGY,
        TYPE_POWER,
    };

    Type_t    type;
    Domain_t  domain;
    uint32_t  flags;
    std::vector<std::string> sources;
};

struct HWMON::SourceDesc_t {
    

    SourceDecl_t::Type_t   type;
    SourceDecl_t::Domain_t domain;
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

    std::vector<SourceDesc_t *> sources;
};

struct HWMON::CollectSources_t {
    std::string prefix;
    std::regex  counterFileRegex;
    std::regex  labelRegex;
    std::pair<std::string, std::string> labelFilename;
};

const std::string HWMON::SUM_ENERGY_LABEL = "Energy consumption [J]";
int HWMON::PROBE_NUMBER;
std::vector<std::string> HWMON::vrSensorLabel;

const std::string HWMON::s_rootPath = "/sys/class/hwmon";
const std::vector<std::string> HWMON::s_checkFiles{ "^energy([0-9]+)_label$", "^power([0-9]+)_oem_info$" };

HWMON::SourceDescMap_t HWMON::s_sourceDesc;
HWMON::SourceNameMap_t HWMON::s_sourceByName;
HWMON::SourceIdVec_t   HWMON::s_sourceById;

std::chrono::nanoseconds             HWMON::s_overflowCheckPeriod(std::chrono::seconds(10));
std::atomic<bool>                    HWMON::s_sampleStoreEnabled(false);
AsyncSampler<HWMON::CounterRecord_t> HWMON::s_asyncSampler;

template<typename T>
double SampleEnergyToPower(const T &sample, uint64_t rangeMax, const T *pPrevSample) {
    if(!pPrevSample)
        return 0.0;

    uint64_t value = (sample.value < pPrevSample->value) ? ((rangeMax - pPrevSample->value) + sample.value) : (sample.value - pPrevSample->value);
    double dt = 1.0f / (double(sample.timestamp - pPrevSample->timestamp) * 1.0e-9);

    return double(value) * dt;
};

void HWMON::init(bool detailed)
{
    initCounters(detailed);
    initSamplers(detailed);
}

void HWMON::close()
{
    closeSamplers();
    s_sourceByName.clear();
}

void HWMON::getCounterValues(CounterRecord_t &record)
{
    s_asyncSampler.Sample(&record);
}

void HWMON::initCounters(bool detailed)
{
    struct SourceDef_t : public SourceDecl_t {
        SourceDef_t(const SourceDecl_t &decl)
            : SourceDecl_t(decl)
        {}

        std::string path;
        std::string baseLabel;
        int id;
        uint64_t period;
    };

    auto GetSuffixLen = [](SourceDecl_t::Domain_t domain) {
        int suffixLen = 0;
        switch(domain)
        {
        case SourceDecl_t::DOMAIN_PER_SOCKET:
            suffixLen = log10(Environment::SOCKETS_PER_NODE) + 1;
            break;
        case SourceDecl_t::DOMAIN_PER_CORE:
            suffixLen = log10(Environment::CORES_PER_SOCKET * Environment::SOCKETS_PER_NODE) + 1;
            break;
        case SourceDecl_t::DOMAIN_PER_THREAD:
            suffixLen = log10(Environment::THREADS_PER_CORE * Environment::CORES_PER_SOCKET * Environment::SOCKETS_PER_NODE) + 1;
            break;
        default:
            break;
        }

        return suffixLen;
    };

    std::map<std::string, SourceDecl_t> sourcesDecl;
    std::vector<SourceDef_t> sourcesDef;

#ifdef _AMD_
    {
        std::vector<std::string> hwmonCounterRoots;

        sysfsutils::IterateDir(s_rootPath, [&](const std::string &filename) {
            std::string name;
            sysfsutils::ReadValue(s_rootPath + "/" + filename + "/name", name);
            if(name == "amd_energy")
            {
                hwmonCounterRoots.push_back(s_rootPath + "/" + filename);
            }
        });

        CollectSources_t collectDesc;
        collectDesc.prefix           = "";
        collectDesc.counterFileRegex = "^energy([0-9]+)_input$";
        collectDesc.labelRegex       = "^([a-zA-Z ]+)([0-9]+)$";
        collectDesc.labelFilename    = { "energy", "_label" };
        sourcesDecl[HWMON_PCKG_ENERGY] = { SourceDecl_t::TYPE_ENERGY, SourceDecl_t::DOMAIN_PER_SOCKET, SourceDecl_t::FLAG_ACCUMULATIVE_BIT };

        if(detailed)
        {
            sourcesDecl[HWMON_CORE_ENERGY] = { SourceDecl_t::TYPE_ENERGY, SourceDecl_t::DOMAIN_PER_CORE, SourceDecl_t::FLAG_NONE_BIT };
        }

        CollectSources(hwmonCounterRoots, collectDesc, sourcesDecl, sourcesDef, 
            [&](SourceDef_t &out, const std::string &path, 
                    const std::smatch &counterMatch, const std::smatch &labelMatch) 
            {
                return true;
            });
        
        int socketCount = 0;
        for(const SourceDef_t &source: sourcesDef)
        {
            socketCount += (source.baseLabel == HWMON_PCKG_ENERGY ? 1 : 0);
        }

        int socketStep = socketCount / Environment::SOCKETS_PER_NODE;
        auto it = sourcesDef.begin();
        while(it != sourcesDef.end())
        {
            if(it->baseLabel == HWMON_PCKG_ENERGY)
            {
                if((it->id % socketStep) != 0)
                {
                    it = sourcesDef.erase(it);
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

        sysfsutils::IterateDir(s_rootPath, [&](const std::string &filename) {
            hwmonCounterRoots.push_back(s_rootPath + "/" + filename);
        });

        CollectSources_t collectDesc;
        collectDesc.prefix           = "/device";
        collectDesc.counterFileRegex = "^power([0-9]+)_average$";
        collectDesc.labelRegex       = "^([a-zA-Z ]+)([0-9]+)$";
        collectDesc.labelFilename    = { "power", "_oem_info" };
        
        sourcesDecl["CPU_Power_Socket"]    = { SourceDecl_t::TYPE_POWER, SourceDecl_t::DOMAIN_PER_SOCKET, SourceDecl_t::FLAG_NONE_BIT };
        sourcesDecl["Module_Power_Socket"] = { SourceDecl_t::TYPE_POWER, SourceDecl_t::DOMAIN_PER_SOCKET, SourceDecl_t::FLAG_NONE_BIT };
        sourcesDecl["SysIO_Power_Socket"]  = { SourceDecl_t::TYPE_POWER, SourceDecl_t::DOMAIN_PER_SOCKET, SourceDecl_t::FLAG_NONE_BIT };
        sourcesDecl["Grace_Power_Socket"]  = { SourceDecl_t::TYPE_POWER, SourceDecl_t::DOMAIN_PER_SOCKET, SourceDecl_t::FLAG_NONE_BIT };
        sourcesDecl["GPU_Power_Socket"]    = { SourceDecl_t::TYPE_POWER, SourceDecl_t::DOMAIN_PER_SOCKET, SourceDecl_t::FLAG_DERIVED_BIT, { "Module_Power_Socket", "Grace_Power_Socket" } };

        CollectSources(hwmonCounterRoots, collectDesc, sourcesDecl, sourcesDef, 
            [&](SourceDef_t &out, const std::string &path, 
                    const std::smatch &counterMatch, const std::smatch &labelMatch) 
            {
                int interval = 0;
                sysfsutils::ReadValue(path + "/power" + counterMatch[1].str() + "_average_interval", interval);
                out.period = interval * 1000000ull;
                return true;
            });
    }

    for(const SourceDef_t &source: sourcesDef)
    {
        int suffixLen = GetSuffixLen(source.domain);

        const std::string fullName  = sysfsutils::MakeCounterName(source.baseLabel, counterLabels, {{ source.id, suffixLen }});
        const std::string fullLabel = sysfsutils::MakeCounterName(source.baseLabel, counterLabels, {{ source.id, suffixLen }}, true);

        SourceDesc_t sourceDesc{
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

        auto baseRes = s_sourceDesc.insert(std::make_pair(source.baseLabel, std::map<int, SourceDesc_t>()));
        auto res     = baseRes.first->second.insert(std::make_pair(source.id, std::move(sourceDesc)));

        s_sourceByName[fullName] = &res.first->second;
    }

    {
        for(auto it = sourcesDecl.begin(); it != sourcesDecl.end(); ++it)
        {
            if((it->second.flags & SourceDecl_t::FLAG_DERIVED_BIT) == 0)
                continue;

            MakeSources(it->second, [&](const SourceDecl_t &sourceDecl, int id) {
                int suffixLen = GetSuffixLen(sourceDecl.domain);

                const std::string fullName  = sysfsutils::MakeCounterName(it->first, counterLabels, {{ id, suffixLen }});
                const std::string fullLabel = sysfsutils::MakeCounterName(it->first, counterLabels, {{ id, suffixLen }}, true);

                SourceDesc_t sourceDesc{
                    sourceDecl.type,
                    sourceDecl.domain,
                    sourceDecl.flags,

                    1.0e-6,
                    fullLabel,
                    fullName,
                    0,
                    0,
                    0,
                    std::vector<Sample_t>(),
                    "",
                    std::ifstream(),
                };

                for(const std::string &sourceName: it->second.sources)
                {
                    auto baseIt = s_sourceDesc.find(sourceName);
                    if(baseIt == s_sourceDesc.end())
                        return;

                    auto sourceIt = baseIt->second.find(id);
                    if(sourceIt == baseIt->second.end())
                        return;

                    sourceDesc.sources.push_back(&sourceIt->second);
                }

                auto baseRes = s_sourceDesc.insert(std::make_pair(it->first, std::map<int, SourceDesc_t>()));
                auto res     = baseRes.first->second.insert(std::make_pair(id, std::move(sourceDesc)));

                s_sourceByName[fullName] = &res.first->second;
            });
        }
    }

    // Set appropriate HWMON data sources to accumulative.
    // NOTE: The "Module_Power_Socket" is top-level power/energy counter for Grace-Hopper, 
    //       but it doesn't exist on Grace-Grace, where "Grace_Power_Socket" is used instead.
    for(const auto &name: std::vector<std::string>{"Module_Power_Socket", "Grace_Power_Socket"})
    {
        auto res = s_sourceDesc.find(name);
        if(res != s_sourceDesc.end())
        {
            for(auto &item: res->second)
            {
                item.second.flags |= SourceDecl_t::FLAG_ACCUMULATIVE_BIT;
            }

            break;
        }
    }
}

double HWMON::getResultValue(CounterValue_t startValue, CounterValue_t stopValue, 
    double runtime, int &cumulative, std::string counter, std::string regionName)
{
    auto it = s_sourceByName.find(counter);
    if(it == s_sourceByName.end())
    {
        return 0.0;
    }

    SourceDesc_t &desc = *it->second;

    cumulative = ((desc.flags & SourceDecl_t::FLAG_ACCUMULATIVE_BIT) != 0) ? 1 : 0;
    uint64_t overflow = stopValue.generation - startValue.generation;
    double res = double(stopValue.value) - double(startValue.value);

    return (res + overflow * double(desc.rangeMax)) * desc.unit;
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
    auto it = s_sourceByName.find(counter);
    if(it != s_sourceByName.end())
    {
        return it->second->label;
    }

    return counter;
}

double HWMON::getUnit(std::string counterName)
{
    auto it = s_sourceByName.find(counterName);
    if(it != s_sourceByName.end())
    {
        return it->second->unit;
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

void HWMON::initSamplers(bool detailed)
{
    for(auto &counter: s_sourceDesc)
    {
        for(auto &item: counter.second)
        {
            s_sourceById.push_back(&item.second);
        }
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
        for(auto &baseDesc: s_sourceDesc)
        {
            for(auto &desc: baseDesc.second)
            {
                timespec now;
                clock_gettime(CLOCK_REALTIME, &now);

                uint64_t value = 0;

                if((desc.second.flags & SourceDecl_t::FLAG_DERIVED_BIT) == 0)
                {
                    sysfsutils::ReadValue(desc.second.fs, value);
                }
                else if(desc.second.sources.size() >= 2) // NOTE: The only derived counter ("GPU_Power_Socket") has 2 operands
                {
                    uint64_t valueA = 0, valueB = 0;
                    sysfsutils::ReadValue(desc.second.sources[0]->fs, valueA);
                    sysfsutils::ReadValue(desc.second.sources[1]->fs, valueB);

                    value = (valueA >= valueB) ? (valueA - valueB) : 0;
                }

                if(isSampling && s_sampleStoreEnabled)
                {
                    desc.second.samples.push_back({
                        uint64_t(now.tv_sec) * 1000000000ull + now.tv_nsec,
                        value
                    });
                }

                if(value < desc.second.lastValue && (desc.second.type & SourceDecl_t::TYPE_ENERGY) != 0)
                    desc.second.overflowCount++;

                desc.second.lastValue = value;
            }
        }

        if(!pRequest)
            return;

        for(auto &baseDesc: s_sourceDesc)
        {
            for(auto &desc: baseDesc.second)
            {
                if((desc.second.type & SourceDecl_t::TYPE_POWER) != 0)
                    continue;
                
                (*pRequest)[desc.second.fullName] = CounterValue_t(desc.second.lastValue, desc.second.overflowCount);
            }
        }
    });
}

void HWMON::closeSamplers()
{
    s_asyncSampler.Stop();
    s_sourceDesc.clear();
    s_sourceById.clear();
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
        if(desc.type == SourceDecl_t::TYPE_ENERGY)
        {
            samples.push_back(SampleEnergyToPower(desc.samples[i], desc.rangeMax, 
                i > 0 ? &desc.samples[i - 1] : nullptr) * desc.unit);
        }
        else
        {
            samples.push_back(double(desc.samples[i].value) * desc.unit);
        }
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
            double power = (double(it->value) * desc.unit);

            if(desc.type == SourceDecl_t::TYPE_ENERGY)
            {
                power = SampleEnergyToPower(*it, desc.rangeMax, 
                    it != desc.samples.begin() ? (it - 1).base() : nullptr) * desc.unit;
            }

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

bool HWMON::isAvailable()
{
#if defined(_INTEL_)
    return false;
#endif

    std::vector<std::pair<std::regex, int> > searchFiles;

    for(const auto &name: s_checkFiles)
    {
        searchFiles.push_back(std::make_pair(std::regex(name), 0));
    }

    sysfsutils::ScanTree(s_rootPath, 3, [&](const std::string &name, const std::string &path) {
        for(std::pair<std::regex, int> &searchFile: searchFiles)
        {
            if(!std::regex_match(name, searchFile.first))
                continue;

            searchFile.second += (access((path).c_str(), R_OK) == 0 ? 1 : 0);
        }
    });

    for(std::pair<std::regex, int> &searchFile: searchFiles)
    {
        if(searchFile.second > 0)
        {
            return true;
        }
    }

    return false;
}

template<typename T, typename F>
void HWMON::CollectSources(const std::vector<std::string> &paths, 
    const CollectSources_t &desc, const std::map<std::string, SourceDecl_t> &sourcesDecl,
    std::vector<T> &sourcesDef, F &&f)
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

            auto it = sourcesDecl.find(baseLabel);
            if(it == sourcesDecl.end())
                return;

            T source(it->second);
            source.path      = path + desc.prefix + "/" + filename;
            source.baseLabel = baseLabel;
            source.id        = sysfsutils::ParseString<int>(hwmonSourceLabelMatch[2]);

            if(f(source, path + desc.prefix, sourceFileMatch, hwmonSourceLabelMatch))
            {
                sourcesDef.push_back(source);
            }
        });
    }
}

template<typename F>
void HWMON::MakeSources(const SourceDecl_t &sourceDecl, F &&f)
{
    auto GetDomainElems = [](SourceDecl_t::Domain_t domain) {
        switch(domain)
        {
        case SourceDecl_t::DOMAIN_PER_SOCKET:
            return Environment::SOCKETS_PER_NODE;
            break;
        case SourceDecl_t::DOMAIN_PER_CORE:
            return (Environment::CORES_PER_SOCKET * Environment::SOCKETS_PER_NODE);
            break;
        case SourceDecl_t::DOMAIN_PER_THREAD:
            return (Environment::THREADS_PER_CORE * Environment::CORES_PER_SOCKET * Environment::SOCKETS_PER_NODE);
            break;
        default:
            return 0;
            break;
        }
    };

    int count = GetDomainElems(sourceDecl.domain);
    for(int i = 0; i < count; ++i)
    {
        f(sourceDecl, i);
    }
}
