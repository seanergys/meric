
#ifndef SRC_STORE_DETAILEDTIMESTORE_H_
#define SRC_STORE_DETAILEDTIMESTORE_H_

#include "timestore.h"

namespace meric {

template <class TStore>
class AggregateStore;

template <class TStore>
class DetailedAggregateStore;

class DetailedTimeStore: public TimeStore <HDEEM>{

	friend class AggregateStore<DetailedTimeStore>;
	friend class DetailedAggregateStore<DetailedTimeStore>;
public:
	DetailedTimeStore(bool samples, bool write = true) : TimeStore(samples, write)
	{
		MERIC_INFO << "Create DetailedTimeStore\n";
	}

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
};

}



#endif /* SRC_STORE_DETAILEDTIMESTORE_H_ */
