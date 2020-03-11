
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
#include "../store/samplesstore.h"
#include "../store/timeenergystore.h"
#include "../store/countersstore.h"
#include "../store/infostore.h"

#include "../wrapper/environmentwrapper.h"
#include "../wrapper/perfeventwrapper.h"
#include "../wrapper/papiwrapper.h"
#include "../wrapper/raplwrapper.h"
#include "../wrapper/msrcounterswrapper.h"

namespace meric {

class Meric {
public:
	static void readRegionParameters();

	virtual void start(const std::string &region)
	{
		for (unsigned int i=0; i<_storeList.size(); i++)
		{
			if (_storeList[i]->getMPIWrite())
				_storeList[i]->addStart(getRegionID(region), currentState.socket[0]); //data storing into file with name according socket#0
		}
		MERIC_INFO << "region '" << region << "' started\n";
	}

	virtual double stop()
	{
		//stop should be in opposite order as start
		//if MERIC_CONTINUAL is used, we want to start it first and stop it last
		for(int i=-1+_storeList.size();i>=0; i--)
		{
			if (_storeList[i]->getMPIWrite())
				_storeList[i]->addEnd(_stack.back());
		}
		MERIC_INFO << "region '" << getRegionName(_stack.size(), _stack.back()) << "' stopped\n";
		if (_storeList.size() && _storeList[0]->getMPIWrite())
			//infostore is allways the first store in the list
			//only the infostore returns the region runtime, other return 0.0
			return _storeList[0]->getRegionDuration();
		else
			return 0.0;
	}
	
	virtual void startIgnore()
	{
#ifdef HAVE_OMP_H
		if (omp_in_parallel())
		{
			if (OMPbarriers)
			{
				#pragma omp barrier
			}
			#pragma omp single nowait
			{
				if (IGNORE)
					std::cerr << "MERIC WARNING: starting nested MERIC ignore sections not supported\n";
				IGNORE=true;
			}
		}
		else
#endif
		{
			if (IGNORE)
				std::cerr << "MERIC WARNING: starting nested MERIC ignore sections not supported\n";
			IGNORE=true;
		}
	}
	
	virtual void stopIgnore()
	{
#ifdef HAVE_OMP_H
		if (omp_in_parallel())
		{
			if (OMPbarriers)
			{
				#pragma omp barrier
			}
			#pragma omp single nowait
			{
				if (!IGNORE)
					std::cerr << "MERIC WARNING: stopping already stopped MERIC ignore section\n";
				IGNORE=false;
			}
		}
		else
#endif
		{
			if (!IGNORE)
				std::cerr << "MERIC WARNING: stopping already stopped MERIC ignore section\n";
			IGNORE=false;
		}
	}

	virtual void save()
	{
		MERIC_INFO << "save stored data\n";
		for(unsigned int i=0; i<_storeList.size(); i++)
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
	
	/**
	 * searching for region name in a list of regions that should be ignored
	 **/
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
			long paramChange = 0;
			//core frequency
			if (next.frequency)
			{
				paramChange = prev.socket[socket].frequency > next.frequency ? prev.socket[socket].frequency-next.frequency : next.frequency-prev.socket[socket].frequency;
				if (paramChange > ignoreSettings.frequency)
				{
					change.socket[socket].frequency = paramChange;
					prev.socket[socket].frequency = next.frequency;
				}
			}
			else
			{
				paramChange = prev.socket[socket].frequency > defaultParameter.frequency ? prev.socket[socket].frequency-defaultParameter.frequency : defaultParameter.frequency-prev.socket[socket].frequency;
				if (paramChange > ignoreSettings.frequency)
				{
					change.socket[socket].frequency = paramChange;
					prev.socket[socket].frequency = defaultParameter.frequency;
				}
			}
			
			//uncore frequency
			if(next.uncoreFrequency)
			{
				paramChange = prev.socket[socket].uncoreFrequency > next.uncoreFrequency ? prev.socket[socket].uncoreFrequency-next.uncoreFrequency : next.uncoreFrequency-prev.socket[socket].uncoreFrequency;
				if (paramChange > ignoreSettings.uncoreFrequency)
				{
					change.socket[socket].uncoreFrequency = paramChange;
					prev.socket[socket].uncoreFrequency = next.uncoreFrequency;
				}
			}
			else
			{
				paramChange = prev.socket[socket].uncoreFrequency > defaultParameter.uncoreFrequency ? prev.socket[socket].uncoreFrequency-defaultParameter.uncoreFrequency : defaultParameter.uncoreFrequency-prev.socket[socket].uncoreFrequency;
				if (paramChange > ignoreSettings.uncoreFrequency)
				{
					change.socket[socket].uncoreFrequency = paramChange;
					prev.socket[socket].uncoreFrequency = defaultParameter.uncoreFrequency;
				}			
			}
			
			//threads
			if(next.nthreads)
			{
				paramChange = prev.socket[socket].nthreads > next.nthreads ? prev.socket[socket].nthreads-next.nthreads : next.nthreads-prev.socket[socket].nthreads;
				if (paramChange > ignoreSettings.nthreads)
				{
					change.socket[socket].nthreads = paramChange;
					prev.socket[socket].nthreads = next.nthreads;
				}
			}
			else
			{
				paramChange = prev.socket[socket].nthreads > defaultParameter.nthreads ? prev.socket[socket].nthreads-defaultParameter.nthreads : defaultParameter.nthreads-prev.socket[socket].nthreads;
				if (paramChange > ignoreSettings.nthreads)
				{
					change.socket[socket].nthreads = paramChange;
					prev.socket[socket].nthreads = defaultParameter.nthreads;
				}			
			}
			
			//power cap
			if (next.powerCapPKG)
			{
				paramChange = prev.socket[socket].powerCapPKG > next.powerCapPKG ? prev.socket[socket].powerCapPKG-next.powerCapPKG : next.powerCapPKG-prev.socket[socket].powerCapPKG;
				if (paramChange > ignoreSettings.powerCapPKG)
				{
					change.socket[socket].powerCapPKG = paramChange;
					prev.socket[socket].powerCapPKG = next.powerCapPKG;
				}
			}
			else
			{
				paramChange = prev.socket[socket].powerCapPKG > defaultParameter.powerCapPKG ? prev.socket[socket].powerCapPKG-defaultParameter.powerCapPKG : defaultParameter.powerCapPKG-prev.socket[socket].powerCapPKG;
				if (paramChange > ignoreSettings.powerCapPKG)
				{
					change.socket[socket].powerCapPKG = paramChange;
					prev.socket[socket].powerCapPKG = defaultParameter.powerCapPKG;
				}
			}

			if (next.powerCapPKGtime)
			{
				paramChange = prev.socket[socket].powerCapPKGtime > next.powerCapPKGtime ? prev.socket[socket].powerCapPKGtime-next.powerCapPKGtime : next.powerCapPKGtime-prev.socket[socket].powerCapPKGtime;
				if (paramChange > ignoreSettings.powerCapPKGtime)
				{
					change.socket[socket].powerCapPKGtime = paramChange;
					prev.socket[socket].powerCapPKGtime = next.powerCapPKGtime;
				}
			}
			else
			{
				paramChange = prev.socket[socket].powerCapPKGtime > defaultParameter.powerCapPKGtime ? prev.socket[socket].powerCapPKGtime-defaultParameter.powerCapPKGtime : defaultParameter.powerCapPKGtime-prev.socket[socket].powerCapPKGtime;
				if (paramChange > ignoreSettings.powerCapPKGtime)
				{
					change.socket[socket].powerCapPKGtime = paramChange;
					prev.socket[socket].powerCapPKGtime = defaultParameter.powerCapPKGtime;
				}
			}

		};
	
		if (parameter.find(region) == parameter.end())	//region name not in list
			for (int socket=0; socket<Environment::SOCKETS_PER_NODE; socket++)
				cmpSettings(settings, defaultParameter, socket);
		else
		{
			std::map<std::pair<int, int>, struct RegionParameters>::iterator it;
			for (int socket=0; socket<Environment::SOCKETS_PER_NODE; socket++)
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
			<< "\tFRQ "<< change.socket[0].frequency << std::endl
			<< "\tunF "<< change.socket[0].uncoreFrequency << std::endl
			<< "\tTHR "<< change.socket[0].nthreads << std::endl
			<< "\tPWR "<< change.socket[0].powerCapPKG << std::endl
			<< "\tTWD "<< change.socket[0].powerCapPKGtime << std::endl
			;
*/
		return change;
	}
	
	/**
	 * Function sets the environment settings at the region start/stop
	 * change - change size in each parameter
	 * MPISelection - frequencies are set by one process per node
	 **/
	void setRegionEnvironment(SocketsRegionParameters change, bool MPIselection=true)
	{
		if (MPIselection)
		{
			//set core frequency
			bool differentSettingsForSockets = false;
			long diffCore = change.socket[0].frequency;
			for (int socket=1; socket < Environment::SOCKETS_PER_NODE; socket++)
			{
				if (diffCore != change.socket[socket].frequency)
					differentSettingsForSockets = true;
			}
			
			if (differentSettingsForSockets)
			{
				for (int socket=0; socket < Environment::SOCKETS_PER_NODE; socket++)
				{
					if(change.socket[socket].frequency)
						Environment::frequency(currentState.socket[socket].frequency, socket);
				}
			}
			else if (diffCore)
				Environment::frequency(currentState.socket[0].frequency);
		
			//set uncore frequncy
			differentSettingsForSockets = false;
			long diffUncore = change.socket[0].uncoreFrequency;
			for (int socket=1; socket < Environment::SOCKETS_PER_NODE; socket++)
			{
				if (diffUncore != change.socket[socket].uncoreFrequency)
					differentSettingsForSockets = true;
			}
			
			if (differentSettingsForSockets)
			{
				for (int socket=0; socket < Environment::SOCKETS_PER_NODE; socket++)
				{
					if(change.socket[socket].uncoreFrequency)
						Environment::uncoreFrequency(currentState.socket[socket].uncoreFrequency, currentState.socket[socket].uncoreFrequency, socket);
				}
			}
			else if (diffUncore)
				Environment::uncoreFrequency(currentState.socket[0].uncoreFrequency, currentState.socket[0].uncoreFrequency);

			//set powercap - to change timeWindow also power must be different
			differentSettingsForSockets = false;
			long diffCapPower = change.socket[0].powerCapPKG;
			long diffCapTime  = change.socket[0].powerCapPKGtime;
			for (int socket=1; socket < Environment::SOCKETS_PER_NODE; socket++)
			{
				if (diffCapPower != change.socket[socket].powerCapPKG || diffCapTime != change.socket[0].powerCapPKGtime)
				{
					differentSettingsForSockets = true;
					break;
				}
			}
			
			if (differentSettingsForSockets)
			{
				for (int socket=0; socket < Environment::SOCKETS_PER_NODE; socket++)
				{
					if(change.socket[socket].powerCapPKG)
						Environment::powercap(currentState.socket[socket].powerCapPKG, currentState.socket[socket].powerCapPKGtime, socket);
				}
			}
			else if (diffCapPower)
				Environment::powercap(currentState.socket[0].powerCapPKG, currentState.socket[0].powerCapPKGtime);
		}
		if(change.socket[0].nthreads)
			Environment::threadNumber(currentState.socket[0].nthreads);	
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
	static bool MPIbarriers;
	static bool OMPbarriers;
	static std::string outputFilenameInfo;

	static int ITERATION;
	static bool CONTINUAL;
	static bool DETAILED;
	static bool AGGREGATE;
	static bool DEBUG;
	static bool SAMPLES;
	static bool IGNORE;
	static bool HDF5;
	static int MODE;
	static std::string COUNTERS;
	static std::string OUTPUT_DIR;
	static std::string OUTPUT_FILENAME;
	static std::string REGION_OPTIONS;
	static std::string BARRIERS;
};

}

#endif /* SRC_MERIC_MERIC_H_ */
