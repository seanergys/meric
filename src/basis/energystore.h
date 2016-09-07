
#ifndef SRC_BASIS_ENERGYSTORE_H_
#define SRC_BASIS_ENERGYSTORE_H_

#include "store.h"

namespace meric {

struct EnergyRecord {
	EnergyRecord(int region, time_t time, double energy)
	: time(time), energy(energy), region(region) {};

	time_t time;
	double energy;
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
