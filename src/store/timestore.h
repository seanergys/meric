
#ifndef SRC_STORE_TIMESTORE_H_
#define SRC_STORE_TIMESTORE_H_

#include "store.h"
#include <unistd.h>

namespace meric {

struct TimeRecord {
	TimeRecord(int region, timespec &time)
	: time(time), region(region) {};

	timespec time;
	int region;
};

template <class TStore>
class AggregateStore;

template <class TStore>
class DetailedAggregateStore;

template <class TSystem> class TimeStore: public Store {

//	enum WRAPPERS {HDEEM, JETSON};

	friend class AggregateStore<TimeStore>;
	friend class DetailedAggregateStore<TimeStore>;
public:
	virtual void addStart(int region);
	virtual void addEnd(int region);
	double getRegionDuration() {return 0.0;};

	void fillRegionStats(RegionStats &stats, size_t iStarts, size_t iEnd);
	void fillRegionStats(DetailedRegionStats &stats, size_t iStarts, size_t iEnd);

	virtual void save(const std::string outputDir, const std::string outputFilename, std::vector<std::string> regionNames, const std::string prefix = "")
	{
		_name = regionNames;
		save(prefix, outputDir, outputFilename, [&] (std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) {
			saveInterval(ss, iStarts, iEnd);
		});
	}

	virtual void save(const std::string prefix, const std::string outputDir, const std::string outputFilename, 
                      std::function<void(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		sleep(2); // wait for HDEEM to load all samples
		computePeriods();

		if(TSystem::getSamples())
			throw 1;
		iterate(_records, prefix, outputDir, outputFilename, processInterval);

		TSystem::deleteSamples();

	}
	virtual void save(std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		sleep(2); // wait for HDEEM to load all samples
		computePeriods();

		if(TSystem::getSamples())
			throw 1;
		iterate(_records, processInterval);
		TSystem::deleteSamples();
	}

/*	TimeStore(const std::string wrapper, bool samples, bool write): Store(samples, write)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);
	}

	TimeStore(const std::string wrapper, bool samples): Store(samples)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);
	}

	TimeStore(const std::string wrapper)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);
	}
*/

	TimeStore(bool samples, bool write): Store(samples, write)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);
	}
		
	TimeStore(bool samples): Store(samples)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);
	}

	TimeStore()
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);
	}


/*	TimeStore(bool write = true): Store(write)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);
	}
*/
protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void computePeriods();

	timespec _start;
	std::vector<TimeRecord> _records;
};

}



#endif /* SRC_STORE_TIMESTORE_H_ */
