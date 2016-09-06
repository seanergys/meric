
#ifndef SRC_BASIS_TIMESTORE_H_
#define SRC_BASIS_TIMESTORE_H_

#include "store.h"

namespace meric {

struct TimeRecord {
	TimeRecord(int region, size_t time)
	: time(time), region(region) {};

	size_t time;
	int region;
};


class TimeStore: public Store {

public:
	void addStart(const std::string &region);
	void addEnd();

private:
	std::vector<TimeRecord> _records;
};

}



#endif /* SRC_BASIS_TIMESTORE_H_ */
