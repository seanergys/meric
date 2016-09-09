
#ifndef SRC_BASIS_ENERGYSTORE_H_
#define SRC_BASIS_ENERGYSTORE_H_

#include "store.h"

namespace meric {

struct EnergyRecord {
	EnergyRecord(int region, timespec &time, double energy)
	: time(time), energy(energy), region(region) {};

	double energy;
	timespec time;
	int region;
};


class EnergyStore: public Store {

public:
	void addStart(const std::string &region);
	void addEnd();

	void save() { iterate(_records); }

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<EnergyRecord> _records;
};

}


#endif /* SRC_BASIS_ENERGYSTORE_H_ */
