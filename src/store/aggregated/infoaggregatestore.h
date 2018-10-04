
#ifndef SRC_STORE_AGGREGATED_INFOAGGREGATESTORE_H_
#define SRC_STORE_AGGREGATED_INFOAGGREGATESTORE_H_

#include "mpi.h"
#include "aggregatestorebase.h"

namespace meric {

template <class TStore>
class InfoAggregateStore: public AggregateStoreBase<TStore> {

public:
	InfoAggregateStore(MPI_Comm &comm): AggregateStoreBase<TStore>(comm)
	{
		_buffer.resize(this->_size);
	}

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void saveInterval(std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<RegionStats> _buffer;
};

}

#include "infoaggregatestore.hpp"



#endif /* SRC_STORE_AGGREGATED_INFOAGGREGATESTORE_H_ */
