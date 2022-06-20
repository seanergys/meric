
#ifndef SRC_MERIC_SHARED_SHAREDMERIC_H_
#define SRC_MERIC_SHARED_SHAREDMERIC_H_

#include "../meric.h"


namespace meric {

class SharedMeric: public Meric {

public:
	
	void start(const std::string &region)
	{
		if (Meric::IGNORE)
			return;

		if (ignoreRegion(region))
		{
			#pragma omp master
			{
				_stack.push_back(getRegionID(region));
				MERIC_INFO << "region '" << region << "' IGNORE start"<< std::endl;
			}
			return;
		}
		
		timespec spectime1, spectime2;
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
		
		#pragma omp single nowait //does not work with _stack, so it does not have to be master thread
		{
			MERIC_INFO << "region '" << region << "' starting\n";
			clock_gettime(CLOCK_REALTIME, &spectime1);
			SocketsRegionParameters change = regionValue(currentState, region);
			setRegionEnvironment(change);
			clock_gettime(CLOCK_REALTIME, &spectime2);
			MERIC_INFO << "MERIC settings changed in " << mutils::timeToDouble(spectime2 - spectime1)*1000 << " ms\n";
		}
		#pragma omp master
		{
			Meric::start(region);
			_stack.push_back(getRegionID(region));
			clock_gettime(CLOCK_REALTIME, &spectime1);
			MERIC_INFO << "MERIC status recorded in " << mutils::timeToDouble(spectime1 - spectime2)*1000 << " ms\n";
		}
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
	}
	
	double stop(const std::string region = "")
	{
		double regionRuntime = 0.0;
		if (Meric::IGNORE)
			return 0.0;
		std::string regionName = getRegionName(_stack.size(), _stack.back());
		if (region != "" && region != regionName)
		{
			#pragma omp single nowait
			std::cerr << "MERIC WARNING: probably corrupted instrumentation\n";
		}
		if (ignoreRegion(regionName))
		{
			#pragma omp master
			{
				MERIC_INFO << "region '" + regionName + "' IGNORE stop" << std::endl;
				_stack.pop_back();
			}
			return 0.0;
		}
		if (OMPbarriers)
		{
			#pragma omp barrier
		}

		#pragma omp master
		{
			MERIC_INFO << "region '" << regionName << "' stopping\n";
			SocketsRegionParameters change = regionValue(currentState, getRegionName(_stack.size()-1, _stack.back()-1));
			setRegionEnvironment(change);
		}

		#pragma omp master
		{
			if (!_stack.size())
			{
				std::cerr << "MERIC ERROR: Incorrect instrumentation. There are more 'ends' than 'starts'!\n";
				regionRuntime = -1.0;
			}
			else
			{
				regionRuntime = Meric::stop();
				_stack.pop_back();
			}
		}
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
		return regionRuntime;
	}
	
	double stopStart(const std::string &startRegionName)
	{
		if (Meric::IGNORE)
			return 0.0;

		double regionRuntime = 0.0;
		const std::string stopRegionName = getRegionName(_stack.size(), _stack.back());
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
		
		#pragma omp master
		{
			if (!ignoreRegion(startRegionName))
			{
				if (!ignoreRegion(stopRegionName))
					MERIC_INFO << "region '" << stopRegionName << "' stopping and staring region '"  << startRegionName << "'\n";
				else
					MERIC_INFO << "starting region '"  << startRegionName << "'\n";
				SocketsRegionParameters change = regionValue(currentState, startRegionName);
				setRegionEnvironment(change);
			}
			else if (!ignoreRegion(stopRegionName))
			{
				MERIC_INFO << "region '" << stopRegionName << "' stopping\n";
				SocketsRegionParameters change = regionValue(currentState, getRegionName(_stack.size()-1, _stack.back()-1));
				setRegionEnvironment(change);
			}
		}
		
		if (!_stack.size())
		{
			#pragma omp single nowait
				std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
			return -1.0;
		}
		#pragma omp master
		{
			if (!ignoreRegion(stopRegionName))
				regionRuntime = Meric::stop();
			_stack.pop_back();

			if (!ignoreRegion(startRegionName))
				Meric::start(startRegionName);
			_stack.push_back(getRegionID(startRegionName));
		}
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
		return regionRuntime;
	}
	
	void startIgnore()
	{
		Meric::startIgnore();
	}
	
	void stopIgnore()
	{
		Meric::stopIgnore();
	}
	
	void save()
	{
		stop(); //applicationMainRegion
		
		if (std::find(MONITORS.begin(), MONITORS.end(), MODE_HDEEM_SAMPLES) != MONITORS.end() || std::find(MONITORS.begin(), MONITORS.end(), MODE_HDEEM_COUNTERS) != MONITORS.end())
		{
			HDEEM::stop(_stopTime);
		}
		else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_DAVIDE) != MONITORS.end())
		{
			DAVIDE::stop(_stopTime);
		}

		Meric::close();
		Environment::restoreSystemSettings();

		if (_stack.size() != 0)
		{
			std::cerr << "MERIC ERROR: not all regions were stopped before the MERIC_Close()\n";
			exit(-1);
		}
		
		//remove ignored regions from the list of regions
		for(const std::string reg : ignoreRegions)
		{
			size_t pos = find(_name.begin(), _name.end(), reg) - _name.begin();
			if (pos < _name.size())
				_name[pos] = "";
		}
		
		unsigned int executionID = 0;
		for(unsigned int i=0; i<_storeList.size(); i++)
		{
			_storeList[i]->initStore(executionID, Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, Meric::outputFormat, Meric::outputFilenameInfo);
			_storeList[i]->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name);
		}
		if (_storeList.size())
			_storeList[0]->storeMeasurementInfo();
	}

	SharedMeric();
	~SharedMeric();
private:

};

}

#endif /* SRC_MERIC_SHARED_SHAREDMERIC_H_ */
