
#ifndef SRC_BASIS_TIMESTORE_H_
#define SRC_BASIS_TIMESTORE_H_

#include "store.h"

namespace meric {

struct TimeRecord {
	TimeRecord(int region, timespec &time)
	: time(time), region(region) {};

	timespec time;
	int region;
};


class TimeStore: public Store {

public:
	void addStart(const std::string &region);
	void addEnd();

	void save()
	{
		clock_gettime(CLOCK_REALTIME, &_end);
		computePeriods();
		HDEEM::getSamples(_samples);
		iterate(_records);
		HDEEM::deleteSamples(_samples);
	}

	TimeStore(): _hdeem_duration(0), _meric_duration(0) { clock_gettime(CLOCK_REALTIME, &_start); }

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void computePeriods();

	std::vector<TimeRecord> _records;

	timespec _start, _end;
	timespec _hdeem_period, _meric_period;
	double _hdeem_duration, _meric_duration;
	hdeem_global_reading_t _samples;
};

}



#endif /* SRC_BASIS_TIMESTORE_H_ */
