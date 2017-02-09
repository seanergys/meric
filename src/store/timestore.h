
#ifndef SRC_BASIS_TIMESTORE_H_
#define SRC_BASIS_TIMESTORE_H_

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

class TimeStore: public Store {

	#define VRDELAY 2	//from list of VR samples we noticed, that VR samples are shifted by two samples

	friend class AggregateStore<TimeStore>;
	friend class DetailedAggregateStore<TimeStore>;
public:
	virtual void addStart(int region);
	virtual void addEnd(int region);

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
		sleep(1); // wait for HDEEM to load all samples
		computePeriods();
		timespec time1, time2;
		clock_gettime(CLOCK_REALTIME, &time1);
		HDEEM::getSamples(_samples);
		clock_gettime(CLOCK_REALTIME, &time2);
		MERIC_INFO << "HDEEM GET SAMPLES: " << mutils::timeToDouble(time2 - time1) <<" s\n";

		iterate(_records, prefix, outputDir, outputFilename, processInterval);

		clock_gettime(CLOCK_REALTIME, &time1);
		HDEEM::deleteSamples(_samples);
		clock_gettime(CLOCK_REALTIME, &time2);
		MERIC_INFO << "HDEEM DELETE SAMPLES: " << mutils::timeToDouble(time2 - time1)<<" s\n";
	}
	virtual void save(std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		sleep(1); // wait for HDEEM to load all samples
		computePeriods();
		timespec time1, time2;
		clock_gettime(CLOCK_REALTIME, &time1);
		HDEEM::getSamples(_samples);
		clock_gettime(CLOCK_REALTIME, &time2);
		MERIC_INFO << "HDEEM GET SAMPLES: " << mutils::timeToDouble(time2 - time1) <<" s\n";

		iterate(_records, processInterval);

		clock_gettime(CLOCK_REALTIME, &time1);
		HDEEM::deleteSamples(_samples);
		clock_gettime(CLOCK_REALTIME, &time2);
		MERIC_INFO << "HDEEM DELETE SAMPLES: " << mutils::timeToDouble(time2 - time1)<<" s\n";

	}

	TimeStore(bool samples, bool write): _hdeem_duration(0), _meric_duration(0), Store(samples, write)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);
	}

	TimeStore(bool samples): _hdeem_duration(0), _meric_duration(0), Store(samples)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);
	}

	TimeStore(): _hdeem_duration(0), _meric_duration(0)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);
	}

/*	TimeStore(bool write = true): _hdeem_duration(0), _meric_duration(0), Store(write)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);
	}
*/
protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void computePeriods();

	timespec _start, _end;
	timespec _hdeem_period, _meric_period;
	double _hdeem_duration, _meric_duration, _hdeem_freq;
	hdeem_global_reading_t _samples;
	size_t _timeAdjustment;

	std::vector<TimeRecord> _records;
};

}



#endif /* SRC_BASIS_TIMESTORE_H_ */
