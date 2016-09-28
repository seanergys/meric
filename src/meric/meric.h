
#ifndef SRC_MERIC_MERIC_H_
#define SRC_MERIC_MERIC_H_

#include <iostream>
#include <map>

#include "../basis/tokenizer.h"
#include "../basis/utils.h"
#include "../store/detailedenergystore.h"
#include "../store/energystore.h"
#include "../store/timestore.h"
#include "../store/detailedtimestore.h"
#include "../store/timeenergystore.h"

#include "../wrapper/environmentwrapper.h"

namespace meric {

struct RegionParameters {
	double frequency;
};

class Meric {

public:
	static void readRegionParameters();

	static bool CONTINUAL;
	static bool DETAILED;
	static bool AGGREGATE;
	static bool DEBUG;
	static std::string OUTPUT_DIR;
    static std::string OUTPUT_FILENAME;

	virtual void start(const std::string &region)
	{
		Environment::frequency(regionValue(region).frequency);
		_store->addStart(region);
	}

	virtual void stop()
	{
		_store->addEnd();
		Environment::frequency(regionValue(_store->region()).frequency);
	}

	virtual void save()
	{
		_store->save( Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME  );
	}

	virtual ~Meric() {};

protected:
	const RegionParameters& regionValue(const std::string &region)
	{
		if (parameter.find(region) != parameter.end()) {
			return parameter[region];
		} else {
			return defaultParameter;
		}
	}

	static std::map<std::string, RegionParameters> parameter;
	static RegionParameters defaultParameter;


	StoreBase * _store;
};

}

#endif /* SRC_MERIC_MERIC_H_ */
