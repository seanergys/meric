
#ifndef SRC_MERIC_SHARED_SHAREDMERIC_H_
#define SRC_MERIC_SHARED_SHAREDMERIC_H_

#include "../meric.h"

#include "../../wrapper/environmentwrapper.h"

namespace meric {

class SharedMeric: public Meric {

public:
	
	void start(const std::string &region)
	{
#ifdef HAVE_OMP_H
		if (omp_in_parallel())
		{
			#pragma omp barrier
			#pragma omp master
			{
				startRegion(region);
			}
		}
		else
#endif
		{
			startRegion(region);
		}
	}
	
	void stop()
	{
#ifdef HAVE_OMP_H
		if (omp_in_parallel())
		{
			#pragma omp barrier
			#pragma omp master
			{
				stopRegion();
			}
		}
		else
#endif
		{
			stopRegion();
		}
	}
	
	void save()
	{
		if (MODE == MODE_HDEEM || MODE == MODE_BOTH)
		{
			HDEEM::stop(_stopTime);
		}
		Meric::close();
		Environment::uncoreFrequency(meric::MAX_UNCORE_FREQ);
		Environment::frequency(meric::MAX_FREQ);

		if (_stack.size() != 0)
		{
			std::cerr << "MERIC ERROR: not all regions were stopped before the MERIC_Close()\n";
			exit -1;
		}
		
		int initCounter = -1;
		for(int i=0; i<_storeList.size(); i++)
		{
			initCounter = _storeList[i]->initStore(initCounter, Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME);
			_storeList[i]->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name);
		}
	}

	SharedMeric();
	~SharedMeric();
private:
	void startRegion(const std::string &region)
	{
		Environment::uncoreFrequency(regionValue(region).uncoreFrequency);
		Environment::frequency(regionValue(region).frequency);
		Environment::threadNumber(regionValue(region).nthreads);
		Meric::start(region);
		_stack.push_back(getRegionID(region));
		return;
	}
	
	void stopRegion()
	{
		if (_stack.size())
		{
			Meric::stop();
			_stack.pop_back();
		}
		else
		{
			std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
		}
		Environment::uncoreFrequency(regionValue(getRegionName(_stack.size(), _stack.back())).uncoreFrequency);
		Environment::frequency(regionValue(getRegionName(_stack.size(), _stack.back())).frequency);
		Environment::threadNumber(regionValue(getRegionName(_stack.size(), _stack.back())).nthreads);
		return;
	}
};

}

#endif /* SRC_MERIC_SHARED_SHAREDMERIC_H_ */
