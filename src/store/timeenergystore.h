
#ifndef SRC_STORE_TIMEENERGYSTORE_H_
#define SRC_STORE_TIMEENERGYSTORE_H_

#include "samplesstore.h"
#include <unistd.h>

namespace meric {

struct TimeEnergyRecord {
	TimeEnergyRecord(int region, timespec &clockTime, std::array<double, HDEEM::PROBE_NUMBER> &energy, RegionParameters configuration)
	: region(region), clockTime(clockTime), energy(energy), configuration(configuration) {};

	int region;
	timespec clockTime;
    std::array<double, HDEEM::PROBE_NUMBER> energy;
	RegionParameters configuration;
};

template <class TStore>
class DebugAggregateStore;

class TimeEnergyStore: public SamplesStore <HDEEM> {

	friend class DebugAggregateStore<TimeEnergyStore>;
public:
	void addStart(int region, RegionParameters configuration);
	void addEnd(int region);
	double getRegionDuration() {return 0.0;};
	void fillRegionStats(DebugRegionStats &stats, size_t iStarts, size_t iEnd);

	void save(const std::string outputDir, const std::string outputFilename, std::vector<std::string> regionNames, const std::string prefix = "")
	{
		_name = regionNames;
		save(prefix, outputDir, outputFilename, [&] (std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) {
			saveInterval(ss, iStarts, iEnd);
		});
	}

	void save(const std::string prefix,
              const std::string outputDir,
              const std::string outputFilename,
              std::function<void(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		sleep(2); // wait for HDEEM to load all samples
		computePeriods();
		if(HDEEM::getSamples() == 0)
		{
			iterate(_records, prefix, outputDir, outputFilename, processInterval);
			HDEEM::deleteSamples();
		}
	}
	void save(std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		sleep(2); // wait for HDEEM to load all samples
		computePeriods();
		if(HDEEM::getSamples() == 0)
		{
			iterate(_records, processInterval);
			HDEEM::deleteSamples();
		}
	}

	TimeEnergyStore(bool samples, bool write) : SamplesStore(samples, write)
	{
		MERIC_INFO << "Create TimeEnergyStore\n";
	}

	TimeEnergyStore(bool samples) : SamplesStore(samples)
	{
		MERIC_INFO << "Create TimeEnergyStore\n";
	}

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<TimeEnergyRecord> _records;
};

}




#endif /* SRC_STORE_TIMEENERGYSTORE_H_ */
