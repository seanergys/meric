
#ifndef SRC_MERIC_MERIC_H_
#define SRC_MERIC_MERIC_H_

#include <iostream>

#include "../basis/utils.h"

#include "../store/detailedenergystore.h"
#include "../store/energystore.h"
#include "../store/timestore.h"
#include "../store/detailedtimestore.h"
#include "../store/timeenergystore.h"

namespace meric {

class Meric {

public:
	static bool CONTINUAL;
	static bool DETAILED;
	static bool DEBUG;
	static std::string OUTPUT_DIR;

	virtual void start(const std::string &region) { _store->addStart(region); }
	virtual void stop() { _store->addEnd(); }
	virtual void save() { _store->save(); }

	Meric();
	virtual ~Meric() { delete _store; }

protected:
	Store * _store;
};

}



#endif /* SRC_MERIC_MERIC_H_ */
