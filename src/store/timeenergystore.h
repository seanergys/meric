
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

	void fillRegionStats(RegionStats &stats, size_t iStarts, size_t iEnd);
	void fillRegionStats(DetailedRegionStats &stats, size_t iStarts, size_t iEnd);

	void save(const std::string prefix = "")
	{
		save(prefix, [&] (std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) {
			saveInterval(ss, iStarts, iEnd);
		});
	}

	void save(const std::string prefix, std::function<void(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		clock_gettime(CLOCK_REALTIME, &_end);
		sleep(1); // wait for HDEEM to load all samples
		computePeriods();
		HDEEM::getSamples(_samples);
		iterate(_records, prefix, processInterval);
		HDEEM::deleteSamples(_samples);
	}
	void save(std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		clock_gettime(CLOCK_REALTIME, &_end);
		sleep(1); // wait for HDEEM to load all samples
		computePeriods();
		HDEEM::getSamples(_samples);
		iterate(_records, processInterval);
		HDEEM::deleteSamples(_samples);
	}

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<TimeEnergyRecord> _records;
};

}




#endif /* SRC_STORE_TIMEENERGYSTORE_H_ */
