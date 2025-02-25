#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cmath>

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
    for(auto &[name, desc]: s_counterDesc)
    {
        uint64_t value = 0;
        ReadValue(desc.fs, value);
        record[name] = value;
    }
}

void HWMON::initMap(std::map<std::string, unsigned long long int> &dict)
{
    namespace fs = std::filesystem;

    const std::string hwmonRootPath("/sys/class/hwmon");

    std::string hwmonCountersRoot;

    for(const auto &entry: fs::directory_iterator(hwmonRootPath))
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
    }

    std::vector<std::pair<std::string, int> > counters;
    counters.push_back({ HWMON_CORE_ENERGY, log10(Environment::CORES_PER_SOCKET * Environment::SOCKETS_PER_NODE) + 1 });
    counters.push_back({ HWMON_DIE_ENERGY, log10(Environment::SOCKETS_PER_NODE) + 1 });

    std::regex counterFileRegex("^energy([0-9]+)_input$");
    std::regex counterLabelRegex("^([a-zA-Z]+)([0-9]+)$");

    for(const auto &entry: fs::directory_iterator(hwmonCountersRoot))
    {
        const std::string filename = entry.path().filename().string();

        std::smatch counterFileMatch;
        if(!std::regex_match(filename, counterFileMatch, counterFileRegex))
            continue;

        std::string hwmonCounterLabel;
        if(!ReadValue(hwmonCountersRoot + "/energy" + counterFileMatch[1].str() + "_label", hwmonCounterLabel))
            continue;

        std::smatch counterLabelMatch;
        if(!std::regex_match(hwmonCounterLabel, counterLabelMatch, counterLabelRegex))
            continue;

        for(const auto &[name, suffixLength]: counters)
        {
            if(name == counterLabelMatch[1].str())
            {
                uint64_t num = 0;
                std::istringstream ss(counterLabelMatch[2].str());
                ss >> num;

                const std::string fullName  = MakeCounterName(name, {{ num, suffixLength }});
                const std::string fullLabel = MakeCounterName(name, {{ num, suffixLength }}, true);

                counterLabel[fullName] = fullLabel;

                dict[fullName] = 0;
                s_counterDesc[fullName] = CounterDesc_t{
                    1.0e-6,
                    hwmonCountersRoot + "/energy" + counterFileMatch[1].str() + "_input",
                    std::ifstream(hwmonCountersRoot + "/energy" + counterFileMatch[1].str() + "_input")
                };

                break;
            }
        }
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
    if(auto it = counterLabel.find(counter); it != counterLabel.end())
        return it->second;

    return counter;
}

double HWMON::getUnit(std::string counterName)
{
    if(auto it = s_counterDesc.find(counterName); it != s_counterDesc.end())
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

    if(auto it = counterLabel.find(name); it != counterLabel.end() && isLabel)
        oss << it->second;
    else
        oss << name;

    for(const auto &[num, width]: nums)
        oss << "_" << std::setfill('0') << std::setw(width) << num;

    if(auto it = counterUnit.find(name); it != counterUnit.end())
        oss << " " << it->second;

    return oss.str();
}