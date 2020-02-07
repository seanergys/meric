
#ifndef SRC_STORE_DETAILEDENERGYSTORE_H_
#define SRC_STORE_DETAILEDENERGYSTORE_H_

#include "store.h"

namespace meric {

struct DetailedEnergyRecord {
	DetailedEnergyRecord(int region, std::array<double, HDEEM::PROBE_NUMBER> &energy, timespec &time, RegionParameters configuration)
	: time(time), energy(energy), region(region), configuration(configuration) {};

	int region;
	std::array<double, HDEEM::PROBE_NUMBER> energy;
	timespec time;
	RegionParameters configuration;
};

template <class TStore>
class AggregateStore;

template <class TStore>
class DetailedAggregateStore;

class DetailedEnergyStore: public Store {

	friend class AggregateStore<DetailedEnergyStore>;
	friend class DetailedAggregateStore<DetailedEnergyStore>;
public:
	void addStart(int region, RegionParameters configuration);
	void addEnd(int region);
	double getRegionDuration() {return 0.0;};

	void fillRegionStats(DetailedRegionStats &stats, size_t iStarts, size_t iEnd)
	{
		for (size_t p = 0; p < HDEEM::PROBE_NUMBER; p++) {
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

	RegionParameters* getConfiguration(size_t id)
	{
		if (_records.size()-1 >= id)
			return &(_records[id].configuration);
		else
		{
			return NULL;
		}
	}

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<DetailedEnergyRecord> _records;
};

}


#endif /* SRC_STORE_DETAILEDENERGYSTORE_H_ */
