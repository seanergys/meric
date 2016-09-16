
#ifndef SRC_BASIS_STORE_H_
#define SRC_BASIS_STORE_H_

#include <vector>
#include <map>
#include <string>

#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include <iomanip>

#include "../wrapper/hdeemwrapper.h"
#include "../basis/utils.h"

namespace meric {

class Store
{
public:
	virtual void addStart(const std::string &region) = 0;
	virtual void addEnd() = 0;

	//virtual void save() = 0;
    
    Store(): _hdeem_duration(0), _meric_duration(0) {
        clock_gettime(CLOCK_REALTIME, &_start);

        std::ifstream iFile("startCounter.tmp");
        if(iFile.is_open()) {
            std::string numOfStartsStr;
            std::getline(iFile, numOfStartsStr);
            _init = std::stoi(numOfStartsStr) + 1;
            iFile.close();

            std::ofstream oFile("startCounter.tmp");
            oFile << _init;
            oFile.close();
        } else {
            std::ofstream oFile("startCounter.tmp");
            oFile << 0;
            oFile.close();
        }

        _jobID = std::stoi(std::getenv("SLURM_JOBID"));
        _stepID = std::stoi(std::getenv("SLURM_STEPID"));
        _nodelist = std::getenv("SLURM_NODELIST");
    };
   
    virtual void computePeriods()
    {
        hdeem_status_t status;
        HDEEM::getStatus(status);
        _hdeem_period = status.stop_time_blade - status.start_time_blade;
        _meric_period = _end - _start;
        _hdeem_duration = _hdeem_period.tv_sec + (double)_hdeem_period.tv_nsec / 1e9;
        _meric_duration = _meric_period.tv_sec + (double)_meric_period.tv_nsec / 1e9;
        std::cout << "measurement error: " << _hdeem_duration - _meric_duration << "\n";
    }

    virtual void save();

	virtual ~Store() {};

protected:
	virtual void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t end) = 0;


	template <class TRecord> void iterate(std::vector<TRecord> &records)
	{
        _counters.resize(_regions.size());
		std::vector<std::ofstream> ss(_regions.size());
		std::vector<std::string> name(_regions.size());
		for (auto it = _regions.begin(); it != _regions.end(); ++it) {
			ss[it->second].open(it->first.c_str(), std::ios::app);
			name[it->second] = it->first;
		}

		std::vector<size_t> starts;
		for (size_t i = 0; i < records.size(); i++) {
			if (!starts.size() || records[starts.back()].region != records[i].region) {
				starts.push_back(i);
				continue;
			}

			saveInterval(ss[records[i].region], starts, i);
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

    size_t _init;
    unsigned int _jobID;
    unsigned int _stepID;
    std::string _nodelist;

    timespec _start, _end;
    timespec _hdeem_period, _meric_period;
    double _hdeem_duration, _meric_duration;
    hdeem_global_reading_t _samples;

};

}

#endif /* SRC_BASIS_STORE_H_ */
