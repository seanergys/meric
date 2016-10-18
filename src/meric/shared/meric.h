
#ifndef SRC_MERIC_SHARED_SHAREDMERIC_H_
#define SRC_MERIC_SHARED_SHAREDMERIC_H_

#include "../meric.h"

namespace meric {

class SharedMeric: public Meric {

public:
	
	void start(const std::string &region)
	{
		Environment::uncoreFrequency(regionValue(region).uncoreFrequency);
		Environment::frequency(regionValue(region).frequency);
		Meric::start(region);
		PERFEVENT::start();
	}
	
	void stop()
	{
		PERFEVENT::stop();
		Environment::uncoreFrequency(regionValue(_store->region()).uncoreFrequency);
		Environment::frequency(regionValue(_store->region()).frequency);
		Meric::stop();
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
