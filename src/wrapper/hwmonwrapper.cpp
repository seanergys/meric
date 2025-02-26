#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
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
std::string SearchDir(const std::string &path, F &&f);

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
    const std::string hwmonRootPath("/sys/class/hwmon");

    std::string hwmonCountersRoot = SearchDir(hwmonRootPath, 
        [&](const std::string &filename) {
            std::string name;
            ReadValue(hwmonRootPath + "/" + filename + "/name", name);
            return (name == "amd_energy");
        });

    /*for(const auto &entry: fs::directory_iterator(hwmonRootPath))
    {
        if(!fs::exists(entry.path().string() + "/name"))
            continue;

        std::string name;
        ReadValue(entry.path().string() + "/name", name);
        if(name == "amd_energy")
        {
            hwmonCountersRoot = entry.path().string();
            break;
        }
    }*/

    std::vector<std::pair<std::string, int> > counters;
    counters.push_back({ HWMON_CORE_ENERGY, log10(Environment::CORES_PER_SOCKET * Environment::SOCKETS_PER_NODE) + 1 });
    counters.push_back({ HWMON_DIE_ENERGY, log10(Environment::SOCKETS_PER_NODE) + 1 });

    std::regex counterFileRegex("^energy([0-9]+)_input$");
    std::regex counterLabelRegex("^([a-zA-Z]+)([0-9]+)$");

    SearchDir(hwmonCountersRoot, [&](const std::string &filename) {
        std::smatch counterFileMatch;
        if(!std::regex_match(filename, counterFileMatch, counterFileRegex))
            return false;

        std::string hwmonCounterLabel;
        if(!ReadValue(hwmonCountersRoot + "/energy" + counterFileMatch[1].str() + "_label", hwmonCounterLabel))
            return false;

        std::smatch counterLabelMatch;
        if(!std::regex_match(hwmonCounterLabel, counterLabelMatch, counterLabelRegex))
            return false;

        for(const auto &counter: counters)
        {
            const auto &name = counter.first;
            const auto &suffixLength = counter.second;

            if(name == counterLabelMatch[1].str())
            {
                uint64_t num = 0;
                std::istringstream ss(counterLabelMatch[2].str());
                ss >> num;

                const std::string fullName  = MakeCounterName(name, {{ num, suffixLength }});
                const std::string fullLabel = MakeCounterName(name, {{ num, suffixLength }}, true);

                dict[fullName] = 0;
                s_counterDesc[fullName] = CounterDesc_t{
                    1.0e-6,
                    hwmonCountersRoot + "/energy" + counterFileMatch[1].str() + "_input",
                    std::ifstream(hwmonCountersRoot + "/energy" + counterFileMatch[1].str() + "_input"),
                    fullLabel
                };

                break;
            }
        }

        return false;
    });
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

	fs >> out;
	return true;
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
std::string SearchDir(const std::string &path, F &&f)
{
    DIR *pDir;
    if((pDir = opendir(path.c_str())) == NULL)
        return std::string();
    
    dirent *pDirEnt;
    while((pDirEnt = readdir(pDir)) != NULL)
    {
        if(f(pDirEnt->d_name))
        {
            closedir(pDir);
            return (path + "/" + pDirEnt->d_name);
        }
    }

    closedir(pDir);
    return std::string();
}