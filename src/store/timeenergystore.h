
#ifndef SRC_STORE_TIMEENERGYSTORE_H_
#define SRC_STORE_TIMEENERGYSTORE_H_

#include "timestore.h"
#include <unistd.h>

namespace meric {

struct TimeEnergyRecord {
	TimeEnergyRecord(int region, timespec &hdeemTime, timespec &clockTime, double energy)
	: region(region), hdeemTime(hdeemTime), clockTime(clockTime), energy(energy) {};

	timespec hdeemTime;
	timespec clockTime;
	double energy;
	int region;
};


class TimeEnergyStore: public TimeStore {

public:
	void addStart(const std::string &region);
	void addEnd();

	virtual void save()
	{
		clock_gettime(CLOCK_REALTIME, &_end);
		sleep(1); // wait for HDEEM to load all samples
		computePeriods();
		HDEEM::getSamples(_samples);
		iterate(_records);
		HDEEM::deleteSamples(_samples);
	}

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<TimeEnergyRecord> _records;
};

}




#endif /* SRC_STORE_TIMEENERGYSTORE_H_ */
