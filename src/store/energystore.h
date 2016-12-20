
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

template <class TStore>
class AggregateStore;

template <class TStore>
class DetailedAggregateStore;

class EnergyStore: public Store {

	friend class AggregateStore<EnergyStore>;
	friend class DetailedAggregateStore<EnergyStore>;
public:
	void addStart(int region);
	void addEnd(int region);

	void fillRegionStats(RegionStats &stats, size_t iStarts, size_t iEnd)
	{
		stats.energy = _records[iEnd].energy - _records[iStarts].energy;
		stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	}

	void save(const std::string outputDir, const std::string outputFilename, std::vector<std::string> regionNames, const std::string prefix = "")
	{
		_name = regionNames;
		HDEEM::stop();
		iterate(_records, prefix, outputDir, outputFilename);
		HDEEM::start();
	}
    
	void save(const std::string prefix, const std::string outputDir, const std::string outputFilename,
              std::function<void(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		HDEEM::stop();
		iterate(_records, prefix, outputDir, outputFilename, processInterval);
		HDEEM::start();
	}

	void save(std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		HDEEM::stop();
		iterate(_records, processInterval);
		HDEEM::start();
	}

	EnergyStore(bool write) : Store(write)
	{
		MERIC_INFO << "Create EnergyStore\n";
	}
	EnergyStore()
	{
		MERIC_INFO << "Create EnergyStore\n";
	}

/*	EnergyStore(bool write = true) : Store(write)
	{
		MERIC_INFO << "Create EnergyStore\n";
	}
*/
protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<EnergyRecord> _records;
};

}


#endif /* SRC_BASIS_ENERGYSTORE_H_ */
