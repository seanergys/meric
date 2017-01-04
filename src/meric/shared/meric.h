
#ifndef SRC_MERIC_SHARED_SHAREDMERIC_H_
#define SRC_MERIC_SHARED_SHAREDMERIC_H_

#include "../meric.h"

#include "../../wrapper/environmentwrapper.h"

namespace meric {

class SharedMeric: public Meric {

public:
	
	void start(const std::string &region)
	{
if (omp_in_parallel())
{
	#pragma omp barrier
	#pragma omp master
	{
			Environment::uncoreFrequency(regionValue(region).uncoreFrequency);
			Environment::frequency(regionValue(region).frequency);
			Environment::threadNumber(regionValue(region).nthreads);
			Meric::start(region);
			_stack.push_back(getRegionID(region));
	}
}
else
{
			Environment::uncoreFrequency(regionValue(region).uncoreFrequency);
			Environment::frequency(regionValue(region).frequency);
			Environment::threadNumber(regionValue(region).nthreads);
			Meric::start(region);
			_stack.push_back(getRegionID(region));
}
	}
	
	void stop()
	{
if (omp_in_parallel())
{
	#pragma omp barrier
	#pragma omp master
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
	}
}
else
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
}
	}
	
	void save()
	{
		int initCounter = -1;
		for(int i=0; i<_storeList.size(); i++)
		{
			initCounter = _storeList[i]->initStore(initCounter, Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME);
			_storeList[i]->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name);
		}
	}

	SharedMeric();
	~SharedMeric();
};

}

#endif /* SRC_MERIC_SHARED_SHAREDMERIC_H_ */
