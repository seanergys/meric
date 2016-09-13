
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

	void fillRegionStats(RegionStats &stats, size_t iStarts, size_t iEnd)
	{
		stats.energy = _records[iEnd].energy[0] - _records[iStarts].energy[0];
		stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	}
	void fillRegionStats(DetailedRegionStats &stats, size_t iStarts, size_t iEnd)
	{
		for (size_t p = 0; p < PROBE_NUMBER; p++) {
			stats.energy[p] = _records[iEnd].energy[p] - _records[iStarts].energy[p];
		}
		stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	}

	void save(const std::string prefix = "") { iterate(_records, prefix); }

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<DetailedEnergyRecord> _records;
};

}


#endif /* SRC_BASIS_DETAILEDENERGYSTORE_H_ */
