
#ifndef SRC_MERIC_MERIC_H_
#define SRC_MERIC_MERIC_H_

#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cmath>

#include "../basis/tokenizer.h"
#include "../basis/utils.h"
#include "../basis/json.h"
#include "../store/detailedenergystore.h"
#include "../store/energystore.h"
#include "../store/timestore.h"
#include "../store/detailedtimestore.h"
#include "../store/timeenergystore.h"
#include "../store/countersstore.h"

#include "../wrapper/perfeventwrapper.h"
#include "../wrapper/papiwrapper.h"


namespace meric {

enum mode {MODE_HDEEM, MODE_RAPL, MODE_BOTH, MODE_RUN};

struct RegionParameters {
	double frequency;
	uint64_t uncoreFrequency;
	int nthreads;
};

struct SocketsRegionParameters {
	RegionParameters socket [2];
};

class Meric {
public:
	static void readRegionParameters();

	static bool CONTINUAL;
	static bool DETAILED;
	static bool AGGREGATE;
	static bool DEBUG;
	static bool SAMPLES;
	static int MODE;
	static std::string COUNTERS;
	static std::string OUTPUT_DIR;
	static std::string OUTPUT_FILENAME;
	static std::string REGION_OPTIONS;
	const int NUMOFSOCKETS = 2;	//TODO get this number from a source

	virtual void start(const std::string &region)
	{
		MERIC_INFO << "region '" << region << "' starting\n";
		for (int i=0; i<_storeList.size(); i++)
		{
			if (_storeList[i]->getMPIWrite())
				_storeList[i]->addStart(getRegionID(region));
		}
		MERIC_INFO << "region '" << region << "' started\n";
	}

	virtual void stop()
	{
		MERIC_INFO << "region '" << getRegionName(_stack.size(), _stack.back()) << "' stopping\n";
		//stop should be in opposite order as start
		//if MERIC_CONTINUAL is used, we want to start it first and stop it last
		for(int i=_storeList.size()-1;i>=0; i--)
		{
			if (_storeList[i]->getMPIWrite())
				_storeList[i]->addEnd(_stack.back());
		}
		MERIC_INFO << "region '" << getRegionName(_stack.size(), _stack.back()) << "' stopped\n";
	}

	virtual void save()
	{
		MERIC_INFO << "save stored data\n";
		for(int i=0; i<_storeList.size(); i++)
			_storeList[i]->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name);
		MERIC_INFO << "store data saved\n";
	}

	virtual void close()
	{
		if (COUNTERS == "perfevent")
			PERFEVENT::close();
		if (COUNTERS == "papi")
			PAPI::close();
		// close() is not needed for RAPL
//		if (meric::Meric::MODE == meric::MODE_RAPL || meric::MODE_BOTH)
//			meric::RAPL::close();

	}

	virtual ~Meric() {};

protected:

	static void readRegionOptions();
	static RegionParameters getRegionSettings(json_object_element_s * data);
	static int getDefaultParameters();
	
	bool ignoreRegion(const std::string &region)
	{
		for (std::vector<std::string>::iterator it=ignoreRegions.begin(); it!=ignoreRegions.end(); it++)
		{
			if (region == *it)
				return true;
		}
		return false;
			
	}
	
	/**
	 * current settings to test if there is a change in values
	 * region name
	 * id of a node for the settings is searching
	 **/
	const SocketsRegionParameters regionValue(SocketsRegionParameters& settings, const std::string &region, int node = -1)
	{
		SocketsRegionParameters change = {};
		if (REGION_OPTIONS == "")
			return change;
		
		auto cmpSettings = [&] (SocketsRegionParameters & prev, RegionParameters & next, int socket)
		{
			//core frequency
			if (next.frequency)
			{
				if (abs(prev.socket[socket].frequency-next.frequency) > ignoreSettings.frequency)
				{
					change.socket[socket].frequency = abs( prev.socket[socket].frequency - next.frequency);
					prev.socket[socket].frequency = next.frequency;
				}
			}
			else
			{
				if (abs(prev.socket[socket].frequency-defaultParameter.frequency) > ignoreSettings.frequency)
				{
					change.socket[socket].frequency = abs( prev.socket[socket].frequency - defaultParameter.frequency);
					prev.socket[socket].frequency = defaultParameter.frequency;
				}
			}
			
			//uncore frequency
			if(next.uncoreFrequency)
			{
				if (abs(prev.socket[socket].uncoreFrequency - next.uncoreFrequency) > ignoreSettings.uncoreFrequency)
				{
					change.socket[socket].uncoreFrequency = abs(prev.socket[socket].uncoreFrequency - next.uncoreFrequency);
					prev.socket[socket].uncoreFrequency = next.uncoreFrequency;
				}
			}
			else
			{
				if (abs(prev.socket[socket].uncoreFrequency-defaultParameter.uncoreFrequency) > ignoreSettings.uncoreFrequency)
				{
					change.socket[socket].uncoreFrequency = abs( prev.socket[socket].uncoreFrequency - defaultParameter.uncoreFrequency);
					prev.socket[socket].uncoreFrequency = defaultParameter.uncoreFrequency;
				}			
			}
			
			//threads
			if(next.nthreads)
			{
				if (abs(prev.socket[socket].nthreads - next.nthreads) > ignoreSettings.nthreads)
				{
					change.socket[socket].nthreads = abs(prev.socket[socket].nthreads - next.nthreads);
					prev.socket[socket].nthreads = next.nthreads;
				}
			}
			else
			{
				if (abs(prev.socket[socket].nthreads-defaultParameter.nthreads) > ignoreSettings.nthreads)
				{
					change.socket[socket].nthreads = abs( prev.socket[socket].nthreads - defaultParameter.nthreads);
					prev.socket[socket].nthreads = defaultParameter.nthreads;
				}			
			}
		};
	
		if (parameter.find(region) == parameter.end())	//region name not in list
			for (int socket=0; socket<NUMOFSOCKETS; socket++)
				cmpSettings(settings, defaultParameter, socket);
		else
		{
			std::map<std::pair<int, int>, struct RegionParameters>::iterator it;
			for (int socket=0; socket<NUMOFSOCKETS; socket++)
			{	
				if (node != -1 && (it = parameter[region].find(std::make_pair(node, socket))) != parameter[region].end())
					cmpSettings(settings, it->second, socket);
				else if (node != -1 && (it = parameter[region].find(std::make_pair(node, -1))) != parameter[region].end())
					cmpSettings(settings, it->second, socket);
				else if ((it = parameter[region].find(std::make_pair(-1, socket))) != parameter[region].end())
					cmpSettings(settings, it->second, socket);
				else if ((it = parameter[region].find(std::make_pair(-1, -1))) != parameter[region].end())
					cmpSettings(settings, it->second, socket);
				else
					cmpSettings(settings, defaultParameter, socket);
			}
		}
/*
		MERIC_INFO << region << " CHANGE\n"
			<< "\tFRQ "<< change.socket[0].frequency	<< std::endl
			<< "\tunF "<< change.socket[0].uncoreFrequency	<< std::endl
			<< "\tTHR "<< change.socket[0].nthreads	<< std::endl;
*/
		return change;
	}
	
	const std::string& getRegionName(int size, int last)
	{
		return size ? _name[last] : _mainRegion;
	}
	
	int getRegionID(const std::string &region)
	{
		size_t pos = find(_name.begin(), _name.end(), region) - _name.begin();
		if (pos >= _name.size())
		{
			pos = _name.size();
			_name.push_back(region);
		}
		return pos;
	}

	//              region_name                     node socket       settings
	static std::map<std::string, std::map<std::pair<int, int>, struct RegionParameters > > parameter;
	static RegionParameters ignoreSettings;
	static std::vector <std::string> ignoreRegions;

	static RegionParameters defaultParameter;
	static SocketsRegionParameters currentState;

	static std::vector<int> _stack;
	static std::vector<std::string> _name;
	std::vector<StoreBase *> _storeList;
	static std::string _mainRegion;
	static timespec _startTime, _stopTime;
};

}

#endif /* SRC_MERIC_MERIC_H_ */
