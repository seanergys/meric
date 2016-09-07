
#ifndef SRC_BASIS_ENERGYSTORE_H_
#define SRC_BASIS_ENERGYSTORE_H_

#include "store.h"

namespace meric {

struct EnergyRecord {
	EnergyRecord(int region, size_t time, size_t energy)
	: time(time), energy(energy), region(region) {};

	size_t time;
	size_t energy;
	int region;
};


class EnergyStore: public Store {

public:
	void addStart(const std::string &region);
	void addEnd();

	~EnergyStore();

private:
	std::vector<EnergyRecord> _records;
};

}


#endif /* SRC_BASIS_ENERGYSTORE_H_ */
