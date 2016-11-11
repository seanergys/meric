
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
	}
	
	void stop()
	{
		Meric::stop();
		Environment::uncoreFrequency(regionValue(_store->region()).uncoreFrequency);
		Environment::frequency(regionValue(_store->region()).frequency);
		Environment::threadNumber(regionValue(_store->region()).nthreads);
	}
	
	void save()
	{
		HDEEM::stop();
		_store->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME);
		HDEEM::start();
	}

	SharedMeric();
	~SharedMeric();
};

}

#endif /* SRC_MERIC_SHARED_SHAREDMERIC_H_ */
