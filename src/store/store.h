
#ifndef SRC_BASIS_STORE_H_
#define SRC_BASIS_STORE_H_

#include <vector>
#include <map>
#include <string>
#include <functional>

#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include <iomanip>

#include "../wrapper/hdeemwrapper.h"
#include "../basis/utils.h"

namespace meric {

struct RegionStats {
	double energy;
	double duration;
};

struct DetailedRegionStats {
	double energy[PROBE_NUMBER];
	double duration;
};

struct DebugRegionStats {
	double energy[PROBE_NUMBER];
	double mericDuration;
	double hdeemDuration;
};

class StoreBase
{
public:
	virtual void addStart(const std::string &region) = 0;
	virtual void addEnd() = 0;

	virtual void save(const std::string outputDir, const std::string outputFilename, const std::string prefix = "") = 0;

	virtual ~StoreBase() {};

protected:
	virtual void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) = 0;

	template <class TRecord> void iterate(std::vector<TRecord> &records, const std::string &prefix, 
                                          const std::string &outputDir,const std::string &outputFilename)
	{
		iterate(records, prefix, outputDir, outputFilename, [&] (std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) {
			saveInterval(ss, iStarts, iEnd);
		});
	}

	template <class TRecord> void iterate(
			std::vector<TRecord> &records,
			const std::string &prefix,
            const std::string &outputDir,
            const std::string &outputFilename,
			std::function<void(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		_jobID = mutils::getenv("SLURM_JOBID", -1);
		_stepID = mutils::getenv("SLURM_STEPID", -1);
		_nodelist = mutils::getenv("SLURM_NODELIST", std::string(""));

		std::ifstream iFile("startCounter.tmp");
		if(iFile.is_open()) {
			std::string numOfStartsStr;
			std::getline(iFile, numOfStartsStr);
			_initCounter = std::stoi(numOfStartsStr) + 1;
			iFile.close();

			std::ofstream oFile("startCounter.tmp");
			oFile << _initCounter;
			oFile.close();
		} else {
			std::ofstream oFile("startCounter.tmp");
			oFile << 0;
			oFile.close();
		}

		_counters.resize(_regions.size());
		std::vector<std::ofstream> ss(_regions.size());
		std::vector<std::string> name(_regions.size());
		for (auto it = _regions.begin(); it != _regions.end(); ++it) {
			std::string folderName = it->first.c_str();
			mutils::createDir(outputDir + "/" + folderName);

			ss[it->second].open(outputDir + "/" + folderName + "/" + (prefix.size() ? prefix + "_" : "" ) + outputFilename + ".csv", std::ios::app);
			name[it->second] = it->first;
		}

		std::vector<size_t> starts;
		for (size_t i = 0; i < records.size(); i++) {
			if (!starts.size() || records[starts.back()].region != records[i].region) {
				starts.push_back(i);
				continue;
			}

			processInterval(ss[records[i].region], starts, i);
			_counters[records[i].region]++;
			_counters.resize(records[i].region + 1);
			_counters.resize(_regions.size());

			starts.pop_back();
		}
	}

	template <class TRecord> void iterate(
			std::vector<TRecord> &records,
			std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		std::vector<size_t> starts;
		_counters.resize(_regions.size());
		for (size_t i = 0; i < records.size(); i++) {
			if (!starts.size() || records[starts.back()].region != records[i].region) {
				starts.push_back(i);
				continue;
			}

			processInterval(starts, i);
			_counters[records[i].region]++;
			_counters.resize(records[i].region + 1);
			_counters.resize(_regions.size());

			starts.pop_back();
		}
	}

	int getRegionID(const std::string &region)
	{
		if (_regions.find(region) == _regions.end()) {
			size_t size = _regions.size();
			_regions[region] = size;
			_name.push_back(region);
		}
		return _regions[region];
	}

	std::map<std::string, int> _regions;
	std::vector<std::string> _name;
	std::vector<int> _stack;
	std::vector<size_t> _counters;

	size_t _initCounter;
	int _jobID;
	int _stepID;
	std::string _nodelist;
};

class Store: public StoreBase {

public:
	virtual void save(const std::string prefix, const std::string outputDir, const std::string outputFile,
                      std::function<void(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)> processInterval) = 0;
	virtual void save(std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval) = 0;

	virtual ~Store() {};
};

}

#endif /* SRC_BASIS_STORE_H_ */
