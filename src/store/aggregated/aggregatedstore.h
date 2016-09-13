
#ifndef SRC_STORE_AGGREGATED_AGGREGATEDSTORE_H_
#define SRC_STORE_AGGREGATED_AGGREGATEDSTORE_H_

#include "mpi.h"
#include "../store.h"

namespace meric {

template <class TStore>
class AggregateStore: public StoreBase {

public:
	void addStart(const std::string &region) { _store.addStart(region); }
	void addEnd() { _store.addEnd(); };

	void save(const std::string prefix = "")
	{
		_store.save(prefix, [&] (std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) {
			saveInterval(ss, iStarts, iEnd);
		});
	}

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	TStore _store;
};

}

#include "aggregatestore.hpp"


#endif /* SRC_STORE_AGGREGATED_AGGREGATEDSTORE_H_ */
