
#ifndef SRC_STORE_AGGREGATED_DEBUGAGGREGATESTORE_H_
#define SRC_STORE_AGGREGATED_DEBUGAGGREGATESTORE_H_

#include <mpi.h>
#include "aggregatestorebase.h"

namespace meric {

template <class TStore>
class DebugAggregateStore: public AggregateStoreBase<TStore> {

public:
	DebugAggregateStore(MPI_Comm &comm, bool write): AggregateStoreBase<TStore>(comm, write)
	{
		_buffer.resize(this->_size);
	}

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void saveInterval(std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<DebugRegionStats> _buffer;
};

}

#include "debugaggregatestore.hpp"



#endif /* SRC_STORE_AGGREGATED_DEBUGAGGREGATESTORE_H_ */
