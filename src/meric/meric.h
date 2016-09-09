
#ifndef SRC_MERIC_MERIC_H_
#define SRC_MERIC_MERIC_H_

#include <iostream>

#include "../basis/detailedenergystore.h"
#include "../basis/utils.h"
#include "../basis/energystore.h"
#include "../basis/timestore.h"
#include "../basis/detailedtimestore.h"

namespace meric {

class Meric {

public:
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
};

}



#endif /* SRC_MERIC_MERIC_H_ */
