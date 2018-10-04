
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
	DetailedTimeStore(bool samples, bool write) : TimeStore(samples, write)
	{
		MERIC_INFO << "Create DetailedTimeStore\n";
	}

	DetailedTimeStore(bool samples) : TimeStore (samples)
	{
		MERIC_INFO << "Create DetailedTimeStore\n";
	}
	
	DetailedTimeStore()
	{
		MERIC_INFO << "Create DetailedTimeStore\n";
	}
protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
};

}



#endif /* SRC_STORE_DETAILEDTIMESTORE_H_ */
