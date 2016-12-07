
#ifndef SRC_MERIC_SHARED_SHAREDMERIC_H_
#define SRC_MERIC_SHARED_SHAREDMERIC_H_

#include "../meric.h"

#include "../../wrapper/environmentwrapper.h"

namespace meric {

class SharedMeric: public Meric {

public:
	
	void start(const std::string &region)
	{
		Environment::uncoreFrequency(regionValue(region).uncoreFrequency);
		Environment::frequency(regionValue(region).frequency);
		Environment::threadNumber(regionValue(region).nthreads);
		Meric::start(region);
		_stack.push_back(getRegionID(region));
	}
	
	void stop()
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
	
	void save()
	{
//		_store->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name);
		for(int i=0; i<_storeList.size(); i++)
			_storeList[i]->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name);
	}

	SharedMeric();
	~SharedMeric();
};

}

#endif /* SRC_MERIC_SHARED_SHAREDMERIC_H_ */
