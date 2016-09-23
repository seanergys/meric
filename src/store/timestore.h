
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


class TimeStore: public Store {

public:
	virtual void addStart(const std::string &region);
	virtual void addEnd();

	void fillRegionStats(RegionStats &stats, size_t iStarts, size_t iEnd);
	void fillRegionStats(DetailedRegionStats &stats, size_t iStarts, size_t iEnd);

	virtual void save(const std::string outputDir, const std::string outputFilename, const std::string prefix = "")
	{
		save(prefix, outputDir, outputFilename, [&] (std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) {
			saveInterval(ss, iStarts, iEnd);
		});
	}

	virtual void save(const std::string prefix, const std::string outputDir, const std::string outputFilename, 
                      std::function<void(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		clock_gettime(CLOCK_REALTIME, &_end);
		sleep(1); // wait for HDEEM to load all samples
		computePeriods();
		HDEEM::getSamples(_samples);
		iterate(_records, prefix, outputDir, outputFilename, processInterval);
		HDEEM::deleteSamples(_samples);
	}
	virtual void save(std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		clock_gettime(CLOCK_REALTIME, &_end);
		sleep(1); // wait for HDEEM to load all samples
		computePeriods();
		HDEEM::getSamples(_samples);
		iterate(_records, processInterval);
		HDEEM::deleteSamples(_samples);
	}

	TimeStore(): _hdeem_duration(0), _meric_duration(0) { clock_gettime(CLOCK_REALTIME, &_start); }

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void computePeriods();

	timespec _start, _end;
	timespec _hdeem_period, _meric_period;
	double _hdeem_duration, _meric_duration;
	hdeem_global_reading_t _samples;

	std::vector<TimeRecord> _records;
};

}



#endif /* SRC_BASIS_TIMESTORE_H_ */
