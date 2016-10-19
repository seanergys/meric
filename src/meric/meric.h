
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
#include "../wrapper/perfeventwrapper.h"

namespace meric {

struct RegionParameters {
	double frequency;
	uint64_t uncoreFrequency;
};

class Meric {

public:
	static void readRegionParameters();

	static bool CONTINUAL;
	static bool DETAILED;
	static bool AGGREGATE;
	static bool DEBUG;
	static bool COUNTERS;
	static std::string OUTPUT_DIR;
    static std::string OUTPUT_FILENAME;

	virtual void start(const std::string &region)
	{
		MERIC_INFO << "region '" << region << "' starting\n";
		_store->addStart(region);
		MERIC_INFO << "region '" << region << "' started\n";
	}

	virtual void stop()
	{
		MERIC_INFO << "region '" << _store->region() << "' stopping\n";
		_store->addEnd();
		MERIC_INFO << "region '" << _store->region() << "' stopped\n";
	}

	virtual void save()
	{
		MERIC_INFO << "save stored data\n";
		_store->save( Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME  );
		MERIC_INFO << "store data saved\n";
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
