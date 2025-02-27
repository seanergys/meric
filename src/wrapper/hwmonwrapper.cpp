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

template<typename T>
bool ReadValue(const std::string &filename, T &out);

template<typename T>
bool ReadValue(std::ifstream &fs, T &out);

template<typename T>
std::string FormatNumber(const T value, int width = 0);

std::string MakeCounterName(const std::string &name, const std::vector<std::pair<int, int>> &nums = {}, 
    bool isLabel = false);

template<typename F>
bool IterateDir(const std::string &path, F &&f);

struct CollectCountersDesc_t {
    std::string prefix;
    std::regex  counterFileRegex;
    std::regex  labelRegex;
    std::pair<std::string, std::string> labelFilename;
    std::set<std::string> counters;
};

template<typename T, typename F>
void CollectCounters(const std::vector<std::string> &paths, const CollectCountersDesc_t &desc, std::vector<T> &counters, F &&f);

template<typename T>
T ParseString(const std::string &str);

std::string &RTrim(std::string &s, const char *t) {
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

std::string &LTrim(std::string &s, const char *t) {
    s.erase(0, s.find_first_not_of(t));
    return s;
}

std::string &Trim(std::string &s, const char *t = " \t\n\r\f\v") {
    return LTrim(RTrim(s, t), t);
}

std::string Trim(const std::string &s, const char *t = " \t\n\r\f\v") {
    std::string sc = s;
    return LTrim(RTrim(sc, t), t);
}

std::string NormalizeLabel(const std::string &str) {
    std::string out = Trim(str);
    std::replace(out.begin(), out.end(), ' ', '_');
    return out;
}

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
        ReadValue(desc.second.fs, value);
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

    {
        std::vector<std::string> hwmonCounterRoots;

        IterateDir(hwmonRootPath, [&](const std::string &filename) {
            std::string name;
            ReadValue(hwmonRootPath + "/" + filename + "/name", name);
            if(name == "amd_energy")
                hwmonCounterRoots.push_back(hwmonRootPath + "/" + filename);
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
                out.samplePeriod = 0.0;
                return true;
            });
        
        labelsSuffixLen[HWMON_CORE_ENERGY] = log10(Environment::CORES_PER_SOCKET * Environment::SOCKETS_PER_NODE) + 1;
        labelsSuffixLen[HWMON_PCKG_ENERGY] = log10(Environment::SOCKETS_PER_NODE) + 1;
        
        int socketCount = 0;
        for(const Counter_t &counter: hwmonCounters)
            socketCount += (counter.baseLabel == HWMON_PCKG_ENERGY ? 1 : 0);

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
                ReadValue(path + "/power" + counterMatch[1].str() + "_average_interval", interval);

                out.samplePeriod = interval * 1.0e-3;

                return true;
            });
    }*/

    for(const Counter_t &counter: hwmonCounters)
    {
        const std::string fullName  = MakeCounterName(counter.baseLabel, {{ counter.id, labelsSuffixLen[counter.baseLabel] }});
        const std::string fullLabel = MakeCounterName(counter.baseLabel, {{ counter.id, labelsSuffixLen[counter.baseLabel] }}, true);

        s_counterDesc[fullName] = CounterDesc_t{
            1.0e-6,
            counter.path,
            std::ifstream(counter.path),
            fullLabel
        };
    }
}

double HWMON::getResultValue(unsigned long long int startValue, unsigned long long int stopValue, 
    double runtime, int &cumulative, std::string counter, std::string regionName)
{
    auto it = s_counterDesc.find(counter);
    if(it == s_counterDesc.end())
        return 0.0;

    // TODO: Handle counter overflow.

    return (stopValue - startValue) * it->second.unit;
}

std::string HWMON::summaryCounter()
{
    return "";
}

std::string HWMON::getSourceLabel()
{
    return "HWMON";
}

std::string HWMON::getCounterLabel(const std::string counter)
{
    auto it = s_counterDesc.find(counter);
    if(it != s_counterDesc.end())
        return it->second.label;

    return counter;
}

double HWMON::getUnit(std::string counterName)
{
    auto it = s_counterDesc.find(counterName);
    if(it != s_counterDesc.end())
        return it->second.unit;

    return -1.0;
}

template<typename T>
bool ReadValue(const std::string &filename, T &out)
{
	std::ifstream fs(filename);
	if(!fs.is_open())
		return false;

	return ReadValue(fs, out);
};

template<typename T>
bool ReadValue(std::ifstream &fs, T &out)
{
	if(!fs.is_open())
		return false;

	fs.seekg(0);
	fs >> out;
	return true;
};

template<>
bool ReadValue(std::ifstream &fs, std::string &out)
{
	if(!fs.is_open())
		return false;

	fs.seekg(0);
	std::getline(fs, out);
	return true;
};

template<typename T>
std::string FormatNumber(const T value, int width)
{
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(width) << value;
    return oss.str();
}

std::string MakeCounterName(const std::string &name, const std::vector<std::pair<int, int>> &nums, 
    bool isLabel)
{
    std::ostringstream oss;

    auto labelIt = isLabel ? counterLabels.find(name) : counterLabels.end();

    if(labelIt != counterLabels.end())
        oss << labelIt->second.first;
    else
        oss << name;

    for(const auto &num: nums)
        oss << "_" << std::setfill('0') << std::setw(num.second) << num.first;
    
    if(labelIt != counterLabels.end())
        oss << " " << labelIt->second.second;

    return oss.str();
}

template<typename F>
bool IterateDir(const std::string &path, F &&f)
{
    DIR *pDir;
    if((pDir = opendir(path.c_str())) == NULL)
        return false;
    
    dirent *pDirEnt;
    while((pDirEnt = readdir(pDir)) != NULL)
        f(pDirEnt->d_name);

    closedir(pDir);
    return true;
}

template<typename T, typename F>
void CollectCounters(const std::vector<std::string> &paths, const CollectCountersDesc_t &desc, std::vector<T> &counters, F &&f)
{
    for(const std::string &path: paths)
    {
        IterateDir(path + desc.prefix, [&](const std::string &filename) {
            std::smatch counterFileMatch;
            if(!std::regex_match(filename, counterFileMatch, desc.counterFileRegex))
                return;

            std::string hwmonCounterLabel;
            if(!ReadValue(path + desc.prefix + "/" + desc.labelFilename.first + counterFileMatch[1].str() + desc.labelFilename.second, hwmonCounterLabel))
                return;

            std::smatch hwmonCounterLabelMatch;
            if(!std::regex_match(hwmonCounterLabel, hwmonCounterLabelMatch, desc.labelRegex))
                return;

            std::string baseLabel = NormalizeLabel(hwmonCounterLabelMatch[1]);

            if(desc.counters.count(baseLabel) < 1)
                return;

            T counter;
            counter.path      = path + "/" + filename;
            counter.baseLabel = baseLabel;
            counter.id        = ParseString<int>(hwmonCounterLabelMatch[2]);

            if(f(counter, path + desc.prefix, counterFileMatch, hwmonCounterLabelMatch))
                counters.push_back(counter);
        });
    }
}

template<typename T>
T ParseString(const std::string &str)
{
    T value = T();
    std::istringstream ss(str);
    ss >> value;
    return value;
}