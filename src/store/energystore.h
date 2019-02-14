
#ifndef SRC_STORE_ENERGYSTORE_H_
#define SRC_STORE_ENERGYSTORE_H_

#include "store.h"

namespace meric {

struct EnergyRecord {
	EnergyRecord(int region, timespec &time, double energy, RegionParameters configuration)
	: time(time), energy(energy), region(region), configuration(configuration) {};

	double energy;
	timespec time;
	int region;
	RegionParameters configuration;
};

template <class TStore>
class AggregateStore;

template <class TStore>
class DetailedAggregateStore;

class EnergyStore: public Store {

	friend class AggregateStore<EnergyStore>;
	friend class DetailedAggregateStore<EnergyStore>;
public:
	void addStart(int region, RegionParameters configuration);
	void addEnd(int region);
	double getRegionDuration() {return 0.0;};
	void fillRegionStats(RegionStats &stats, size_t iStarts, size_t iEnd);

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

	EnergyStore(bool samples, bool write) : Store(samples, write)
	{
		MERIC_INFO << "Create EnergyStore\n";
	}
	EnergyStore(bool samples)
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


#endif /* SRC_STORE_ENERGYSTORE_H_ */
