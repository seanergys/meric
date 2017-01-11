
#ifndef SRC_STORE_TIMEENERGYSTORE_H_
#define SRC_STORE_TIMEENERGYSTORE_H_

#include "timestore.h"
#include <unistd.h>

namespace meric {

struct TimeEnergyRecord {
	TimeEnergyRecord(int region, timespec &hdeemTime, timespec &clockTime, std::array<double, PROBE_NUMBER> &energy)
	: region(region), hdeemTime(hdeemTime), clockTime(clockTime), energy(energy) {};

	timespec hdeemTime;
	timespec clockTime;
    std::array<double, PROBE_NUMBER> energy;
	int region;
};

template <class TStore>
class DebugAggregateStore;

class TimeEnergyStore: public TimeStore {

	friend class DebugAggregateStore<TimeEnergyStore>;
public:
	void addStart(int region);
	void addEnd(int region);

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
		clock_gettime(CLOCK_REALTIME, &_end);
		sleep(1); // wait for HDEEM to load all samples
		computePeriods();
		HDEEM::getSamples(_samples);
		iterate(_records, prefix, outputDir, outputFilename, processInterval);
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

	TimeEnergyStore(bool write) : TimeStore(write)
	{
		MERIC_INFO << "Create TimeEnergyStore\n";
	}

	TimeEnergyStore()
	{
		MERIC_INFO << "Create TimeEnergyStore\n";
	}

/*	TimeEnergyStore(bool write = true) : TimeStore(write)
	{
		MERIC_INFO << "Create TimeEnergyStore\n";
	}
*/
protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<TimeEnergyRecord> _records;
};

}




#endif /* SRC_STORE_TIMEENERGYSTORE_H_ */
