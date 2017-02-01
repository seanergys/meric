
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

template <class TStore>
class AggregateStore;

template <class TStore>
class DetailedAggregateStore;

class DetailedEnergyStore: public Store {

	friend class AggregateStore<DetailedEnergyStore>;
	friend class DetailedAggregateStore<DetailedEnergyStore>;
public:
	void addStart(int region);
	void addEnd(int region);

	void fillRegionStats(DetailedRegionStats &stats, size_t iStarts, size_t iEnd)
	{
		for (size_t p = 0; p < PROBE_NUMBER; p++) {
			stats.energy[p] = _records[iEnd].energy[p] - _records[iStarts].energy[p];
		}
		stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	}

	void save(const std::string outputDir, const std::string outputFilename, std::vector<std::string> regionNames, const std::string prefix = "")
	{
		_name = regionNames;
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

	DetailedEnergyStore(bool samples, bool write) : Store(samples, write)
	{
		MERIC_INFO << "Create DetailedEnergyStore\n";
	}

	DetailedEnergyStore(bool samples) : Store(samples)
	{
		MERIC_INFO << "Create DetailedEnergyStore\n";
	}

	DetailedEnergyStore()
	{
		MERIC_INFO << "Create DetailedEnergyStore\n";
	}

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<DetailedEnergyRecord> _records;
};

}


#endif /* SRC_BASIS_DETAILEDENERGYSTORE_H_ */
