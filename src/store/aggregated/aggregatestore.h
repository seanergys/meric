
#ifndef SRC_STORE_AGGREGATED_AGGREGATESTORE_H_
#define SRC_STORE_AGGREGATED_AGGREGATESTORE_H_

#include <mpi.h>
#include "aggregatestorebase.h"

namespace meric {

template <class TStore>
class AggregateStore: public AggregateStoreBase<TStore> {

public:
	typedef typename TStore::samplesType StoreSamplesType;
//	typedef TSystem samplesType;

	AggregateStore(MPI_Comm &comm, bool write): AggregateStoreBase<TStore>(comm, write)
	{
		_buffer.resize(this->_size);
	}

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void saveInterval(std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<RegionStats> _buffer;
};

}

#include "aggregatestore.hpp"


#endif /* SRC_STORE_AGGREGATED_AGGREGATESTORE_H_ */
