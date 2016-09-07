
#ifndef SRC_MERIC_MERIC_H_
#define SRC_MERIC_MERIC_H_

#include <iostream>
#include "../basis/utils.h"
#include "../basis/detailedstore.h"
#include "../basis/energystore.h"
#include "../basis/timestore.h"

namespace meric {

class Meric {

public:
	static bool CONTINUAL;
	static bool DETAILED;
	static std::string OUTPUT_DIR;

	virtual void start(const std::string &region);
	virtual void stop();

	Meric() {};
	virtual ~Meric() {};

protected:
	TimeStore _timeStore;
	EnergyStore _energyStore;
	DetailedStore _detailedStore;
};

}



#endif /* SRC_MERIC_MERIC_H_ */
