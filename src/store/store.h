
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
#include "../wrapper/jetsonwrapper.h"
#include "../basis/utils.h"

namespace meric {

struct RegionStats {
	double energy;
	double duration;
};

struct DetailedRegionStats {
	double energy[HDEEM::PROBE_NUMBER];											//TODO odstranit zavislost na HDEEM ?
	double duration;
};

struct DebugRegionStats {
	double energyStatsStructure[HDEEM::PROBE_NUMBER];
	double energy[HDEEM::PROBE_NUMBER];
	double mericDuration;
	double hdeemDuration;
};

class StoreBase
{
public:
	virtual void addStart(int region) = 0;
	virtual void addEnd(int region) = 0;
	virtual void save(const std::string outputDir, const std::string outputFilename, std::vector<std::string> regionNames, const std::string prefix = "") = 0;
	StoreBase(bool samples = false) : printSamples(samples) {};
	StoreBase(bool samples, bool write) : printSamples(samples), MPIProcessWrite(write) {};
//	StoreBase(bool write = true) : MPIProcessWrite(write) {};
	virtual ~StoreBase() {};
	void setRegionsNames(std::vector<std::string> name) {_name = name;};
	bool getMPIWrite() {return MPIProcessWrite;};
	void setMPIWrite(bool write) {MPIProcessWrite = write;};
	
	int initStore(	int counter)
	{
		_jobID = mutils::getenv("SLURM_JOBID", -1);
		_stepID = mutils::getenv("SLURM_STEPID", -1);
		_nodelist = mutils::getenv("SLURM_NODELIST", std::string(""));
		_initCounter = counter;
	};
	
	int initStore(	int counter,
					const std::string outputDir,
					const std::string outputFilename,
					const std::string prefix = "")
	{
		_jobID = mutils::getenv("SLURM_JOBID", -1);
		_stepID = mutils::getenv("SLURM_STEPID", -1);
		_nodelist = mutils::getenv("SLURM_NODELIST", std::string(""));
		
		if(counter >= 0)
		{
			_initCounter = counter;
			return counter;
		}	
		
		//CALLTREE init counter
		mutils::createDir(outputDir + "Counters");
        std::string startCounterFile = outputDir + "Counters/" + "startCounter_" + (prefix.size() ? prefix + "_" : "" )  + outputFilename + ".tmp";

		std::ifstream iFile(startCounterFile);
		if(iFile.is_open())
		{
			std::string numOfStartsStr;
			std::getline(iFile, numOfStartsStr);
			_initCounter = std::stoi(numOfStartsStr) + 1;
			iFile.close();

		}else
		{
			_initCounter = 0;
		}
		std::ofstream oFile(startCounterFile);
		oFile << _initCounter;
		oFile.close();
		return _initCounter;
	};
	
protected:
	virtual void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) = 0;

	template <class TRecord> void iterate(std::vector<TRecord> &records, const std::string &prefix, 
                                          const std::string &outputDir,const std::string &outputFilename)
	{
		iterate(records, prefix, outputDir, outputFilename, [&] (std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) 
		{
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
		_counters.resize(_name.size());
		_levels.resize(_name.size());
		timespec time1, time2;
		clock_gettime(CLOCK_REALTIME, &time1);
		std::vector<std::ofstream> ss(_name.size());
		for (int i=0; i<_name.size(); i++)
		{
			std::string folderName = _name[i].c_str();
			mutils::createDir(outputDir + "/" + folderName);
			ss[i].open(outputDir + "/" + folderName + "/" + (prefix.size() ? prefix + "_" : "" ) + outputFilename + ".csv", std::ios::app);
		}
		clock_gettime(CLOCK_REALTIME, &time2);
		MERIC_INFO << "MERIC open csv files in: " << mutils::timeToDouble(time2 - time1) <<" s\n";
		
		std::vector<size_t> starts;
		for (size_t i = 0; i < records.size(); i++)
		{
			if (!starts.size() || records[starts.back()].region != records[i].region)
			{
//				if (_levels[records[i].region] && _levels[records[i].region]!=starts.size())
//					std::cerr << "MERIC WARNING: region "<< _name[records[i].region] << " is called more than once in one call tree, check the call tree if everything is ok\n";
				_levels[records[i].region] = starts.size();
				starts.push_back(i);
			}
			else
			{
				processInterval(ss[records[i].region], starts, i);
				_counters[records[i].region]++;		
				for(size_t j=0; j<_levels.size(); j++)
				{
					if (_levels[j] > _levels[records[i].region])
						_counters[j] = 0;
				}
				starts.pop_back();
			}
		}
	}


	template <class TRecord> void iterate(
			std::vector<TRecord> &records,
			std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		std::vector<size_t> starts;
		_counters.resize(_name.size());
		_levels.resize(_name.size());
		for (size_t i = 0; i < records.size(); i++) 
		{
			if (!starts.size() || records[starts.back()].region != records[i].region)
			{
//				if (_levels[records[i].region])
//					std::cerr << "MERIC ERROR: region "<< _name[records[i].region] << " is called more than once in one call tree, check the call tree if everything is ok\n";
				_levels[records[i].region] = starts.size();
				starts.push_back(i);
			}
			else
			{
				processInterval(starts, i);
				_counters[records[i].region]++;
				for(size_t j=0; j<_levels.size(); j++)
				{
					if (_levels[j] > _levels[records[i].region])
						_counters[j] = 0;
				}
				starts.pop_back();
			}
		}
	}

	std::vector<std::string> _name;
	std::vector<size_t> _counters;
	std::vector<size_t> _levels;

	size_t _initCounter;
	int _jobID;
	int _stepID;
	std::string _nodelist;
	bool MPIProcessWrite;
	bool printSamples;
};

class Store: public StoreBase {

public:
	virtual void save(const std::string prefix, const std::string outputDir, const std::string outputFile,
                      std::function<void(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)> processInterval) = 0;
	virtual void save(std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval) = 0;
	Store() : StoreBase() {};
	Store(bool samples) : StoreBase(samples) {};
	Store(bool samples, bool write) : StoreBase(samples, write) {};
//	Store(bool samples, bool write = true) : StoreBase(samples, write) {};
	virtual ~Store() {};
};

}

#endif /* SRC_BASIS_STORE_H_ */
