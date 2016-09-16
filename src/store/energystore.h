
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

    void save() {
        clock_gettime(CLOCK_REALTIME, &_end);
        sleep(1); // wait for HDEEM to load all samples
        computePeriods();
        std::cout << "TEST 3" << std::endl;
        HDEEM::getSamples(_samples);
        std::cout << "TEST 4" << std::endl;

        iterate(_records);
        HDEEM::deleteSamples(_samples);
    }

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<EnergyRecord> _records;
};

}


#endif /* SRC_BASIS_ENERGYSTORE_H_ */
