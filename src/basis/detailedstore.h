
#ifndef SRC_BASIS_DETAILEDSTORE_H_
#define SRC_BASIS_DETAILEDSTORE_H_

#include "store.h"

namespace meric {

struct DetailedRecord {
	DetailedRecord(int region, size_t time)
	: time(time), region(region) {};

	size_t time;
	size_t energy[7];
	int region;
};


class DetailedStore: public Store {

public:
	void addStart(const std::string &region);
	void addEnd();

	~DetailedStore();

private:
	std::vector<DetailedRecord> _records;
};

}


#endif /* SRC_BASIS_DETAILEDSTORE_H_ */
