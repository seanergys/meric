
#ifndef SRC_STORE_SAMPLESSTORE_H_
#define SRC_STORE_SAMPLESSTORE_H_

#include "store.h"
#include <unistd.h>

namespace meric {

struct SamplesRecord {
	SamplesRecord(int region, timespec &time, RegionParameters configuration)
	: time(time), region(region), configuration(configuration) {};

	timespec time;
	int region;
	RegionParameters configuration;
};

template <class TStore>
class AggregateStore;

template <class TStore>
class DetailedAggregateStore;

template <class TSystem> class SamplesStore: public Store {

	friend class AggregateStore<SamplesStore>;
	friend class DetailedAggregateStore<SamplesStore>;
public:
//	typedef typename TSystem::samplesType StoreSamplesType;
	typedef TSystem samplesType;
	
	virtual void addStart(int region, RegionParameters configuration);
	virtual void addEnd(int region);
	double getRegionDuration() {return 0.0;};
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

		if(TSystem::getSamples() == 0)
		{
			iterate(_records, prefix, outputDir, outputFilename, processInterval);
			TSystem::deleteSamples();
		}
	}
	virtual void save(std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		sleep(2); // wait for HDEEM to load all samples
		computePeriods();

		if(TSystem::getSamples() == 0)
		{
			iterate(_records, processInterval);
			TSystem::deleteSamples();
		}
	}
	
	SamplesStore(bool samples, bool write = true): Store(samples, write)
	{
		MERIC_INFO << "Create SamplesStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);
	}

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void computePeriods();

	timespec _start;
	std::vector<SamplesRecord> _records;
};

}



#endif /* SRC_STORE_SAMPLESSTORE_H_ */
