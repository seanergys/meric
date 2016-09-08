
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

	TimeStore() { clock_gettime(CLOCK_REALTIME, &_start); }
	~TimeStore();

protected:
	timespec _start, _end;
	std::vector<TimeRecord> _records;
};

}



#endif /* SRC_BASIS_TIMESTORE_H_ */
