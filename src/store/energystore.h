
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

	void fillRegionStats(RegionStats &stats, size_t iStarts, size_t iEnd)
	{
		stats.energy = _records[iEnd].energy - _records[iStarts].energy;
		stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	}
	void fillRegionStats(DetailedRegionStats &stats, size_t iStarts, size_t iEnd)
	{
		stats.energy[0] = _records[iEnd].energy - _records[iStarts].energy;
		stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	}

	void save(const std::string outputDir, const std::string outputFilename, const std::string prefix = "") { 
        iterate(_records, prefix, outputDir, outputFilename); 
    }
	void save(const std::string prefix, const std::string outputDir, const std::string outputFilename,
              std::function<void(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		iterate(_records, prefix, outputDir, outputFilename, processInterval);
	}
	void save(std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		iterate(_records, processInterval);
	}

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<EnergyRecord> _records;
};

}


#endif /* SRC_BASIS_ENERGYSTORE_H_ */
