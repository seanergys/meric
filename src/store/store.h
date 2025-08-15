
#ifndef SRC_STORE_STORE_H_
#define SRC_STORE_STORE_H_

#include <vector>
#include <map>
#include <list>
#include <string>
#include <functional>
#include <numeric>

#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <exception> 

#include "../wrapper/environmentwrapper.h"
#include "../wrapper/hdeemwrapper.h"
#include "../wrapper/jetsonwrapper.h"
#include "../wrapper/thunderwrapper.h"
#include "../wrapper/davidewrapper.h"
#include "../wrapper/hwmonwrapper.h"
#include "../wrapper/raplwrapper.h"
#include "../basis/utils.h"

namespace meric {

struct InfoStats { //infoAggregateStore
	double duration;
	double startTime;
};

struct DetailedRegionStats { //aggregateStore
	double energy[HDEEM::PROBE_NUMBER];											//TODO remove dependency on HDEEM !
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
	virtual void addStart(int region, RegionParameters configuration) = 0;
	virtual void addEnd(int region) = 0;
	virtual double getRegionDuration() = 0;
	virtual void loadSchedulerEnvironment() = 0;	
	virtual void save(const std::string outputDir, const std::string outputFilename, std::vector<std::string> regionNames, const std::string prefix = "") = 0;

	StoreBase(bool samples, bool write) : printSamples(samples), MPIProcessWrite(write) {};
	virtual ~StoreBase() {};
	void setRegionsNames(std::vector<std::string> name) {_name = name;};
	bool getMPIWrite() {return MPIProcessWrite;};
	void setMPIWrite(bool write) {MPIProcessWrite = write;};

	virtual void initStore(	unsigned int & counter, const RegionParameters &outputFormat)
	{
		loadSchedulerEnvironment();
		_initCounter = counter;
		outputFilenameFormat = outputFormat;
	};
	
	virtual void initStore(	unsigned int & counter,
					const std::string outputDir,
					const std::string outputFilename,
					const RegionParameters &outputFormat,
					const std::string outputFilenameInfo, //node_CF_UnCF_config
					const std::string prefix = "")
	{
		loadSchedulerEnvironment();
		_initCounter = counter;
		outputFilenameFormat = outputFormat;
		return;
	};
	
	virtual void storeMeasurementInfo() {};

	
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
		for (std::string folderName : _name)
		{
			if (folderName != "")
				mutils::createDir(outputDir + "/" + folderName);
		}
		
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
			else if (_name[records[i].region] == "")	// "record" of an ignored region
			{
				_counters[records[i].region]++;
				for(size_t j=0; j<_levels.size(); j++)
				{
					if (_levels[j] > _levels[records[i].region])
						_counters[j] = 0;
				}
				starts.pop_back();
			}
			else
			{
				std::string filenamePrefix = (prefix.size() ? prefix + "_" : "" )
					+ (outputFilenameFormat.frequency == 0  ? "" : std::to_string(records[starts.back()].configuration.frequency)  +"_" )
					+ (outputFilenameFormat.uncoreFrequency == 0 ? "" : std::to_string(records[starts.back()].configuration.uncoreFrequency) +"_" )
					+ (outputFilenameFormat.nthreads == 0 ? "" : std::to_string(records[starts.back()].configuration.nthreads) +"_" )
					+ (outputFilenameFormat.powerCapPKG == 0 ? "" : std::to_string(records[starts.back()].configuration.powerCapPKG) +"_" )
					+ (outputFilenameFormat.powerCapPKGtime == 0 ? "" : std::to_string(records[starts.back()].configuration.powerCapPKGtime) +"_" )
					+ (outputFilenameFormat.frequencyGPU == 0  ? "" : std::to_string(records[starts.back()].configuration.frequencyGPU)  +"_" )
					+ (outputFilenameFormat.frequencyGPUmem == 0  ? "" : std::to_string(records[starts.back()].configuration.frequencyGPUmem)  +"_" )
					+ (outputFilenameFormat.powerCapGPU == 0 ? "" : std::to_string(records[starts.back()].configuration.powerCapGPU) +"_" )					
					+ outputFilename;
				if (filenamePrefix.back() == '_')
					filenamePrefix.pop_back();
				std::ofstream ss(outputDir + "/" + _name[records[i].region] + "/" + filenamePrefix + ".csv", std::ios::app);
				processInterval(ss, starts, i);
				_counters[records[i].region]++;
				for(size_t j=0; j<_levels.size(); j++)
				{
					if (_levels[j] > _levels[records[i].region])
						_counters[j] = 0;
				}
				starts.pop_back();
				ss.close();
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
			else if (_name[records[i].region] == "")	// "record" of an ignored region
			{
				_counters[records[i].region]++;
				for(size_t j=0; j<_levels.size(); j++)
				{
					if (_levels[j] > _levels[records[i].region])
						_counters[j] = 0;
				}
				starts.pop_back();
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
	unsigned int _stepID;
	std::string _nodelist;
	bool printSamples;
	bool MPIProcessWrite;
	RegionParameters outputFilenameFormat;
};

class Store: public StoreBase {

public:
	
	void loadSchedulerEnvironment()
	{
		_jobID = mutils::getenv("SLURM_JOBID", 0);
		if (_jobID == -1)
		{
			_jobID = mutils::getenv("PBS_JOBID", 0);
			if (_jobID != -1)
			{
				_stepID = mutils::getenv("PBS_TASKNUM", 0);
				_nodelist = mutils::getenv("PBS_NODEFILE", std::string(""));
				if (_nodelist != "")
				{
					std::list<std::string> nodes;
					std::ifstream iFile(_nodelist);
					if(iFile.is_open())
					{
						std::string data;
						for(std::string data; std::getline(iFile, data);)
						{
							nodes.push_back(data);
						}
						iFile.close();
					}

					nodes.unique();
					_nodelist = "";
					_nodelist = std::accumulate(std::begin(nodes), std::end(nodes),
					            std::string(),
					            [](std::string &a, std::string &b) {
									return a.empty() ? b : a + ';' + b;
								});
				}
			}
		}
		else
		{
			_stepID = mutils::getenv("SLURM_STEPID", 0);
			_nodelist = mutils::getenv("SLURM_NODELIST", std::string(""));
		}
	}

	
	virtual void save(const std::string prefix, const std::string outputDir, const std::string outputFile,
                      std::function<void(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)> processInterval) = 0;
	virtual void save(std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval) = 0;
	Store(bool samples) : StoreBase(samples, true) {};
	Store(bool samples, bool write) : StoreBase(samples, write) {};
	virtual ~Store() {};
};

}

#endif /* SRC_STORE_STORE_H_ */
