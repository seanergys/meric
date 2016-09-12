
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

namespace meric {

struct RegionParameters {
	double frequency;
};

class Meric {

public:
	static void readRegionParameters();

	static bool CONTINUAL;
	static bool DETAILED;
	static std::string OUTPUT_DIR;

	virtual void start(const std::string &region) { _store->addStart(region); }
	virtual void stop() { _store->addEnd(); }
	virtual void save() { _store->save(); }

	Meric();
	virtual ~Meric() { delete _store; }

protected:
	Store * _store;
	static std::map<std::string, RegionParameters> parameter;
};

}



#endif /* SRC_MERIC_MERIC_H_ */
