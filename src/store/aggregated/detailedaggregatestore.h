
#ifndef SRC_STORE_AGGREGATED_DETAILEDAGGREGATESTORE_H_
#define SRC_STORE_AGGREGATED_DETAILEDAGGREGATESTORE_H_

#include "mpi.h"
#include "aggregatestorebase.h"

namespace meric {

template <class TStore>
class DetailedAggregateStore: public AggregateStoreBase<TStore> {

public:
//	DetailedAggregateStore(MPI_Comm &comm, bool write): AggregateStoreBase<TStore>(comm, write)
	DetailedAggregateStore(MPI_Comm &comm): AggregateStoreBase<TStore>(comm)
	{
		_buffer.resize(this->_size);
	};


protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void saveInterval(std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<DetailedRegionStats> _buffer;
};

}

#include "detailedaggregatestore.hpp"




#endif /* SRC_STORE_AGGREGATED_DETAILEDAGGREGATESTORE_H_ */
