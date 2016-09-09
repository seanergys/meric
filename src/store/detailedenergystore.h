
#ifndef SRC_BASIS_DETAILEDENERGYSTORE_H_
#define SRC_BASIS_DETAILEDENERGYSTORE_H_

#include "store.h"

namespace meric {

struct DetailedEnergyRecord {
	DetailedEnergyRecord(int region, std::array<double, PROBE_NUMBER> &energy, timespec &time)
	: time(time), energy(energy), region(region) {};

	std::array<double, PROBE_NUMBER> energy;
	timespec time;
	int region;
};


class DetailedEnergyStore: public Store {

public:
	void addStart(const std::string &region);
	void addEnd();

	void save() { iterate(_records); }

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<DetailedEnergyRecord> _records;
};

}


#endif /* SRC_BASIS_DETAILEDENERGYSTORE_H_ */
