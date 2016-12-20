
#ifndef SRC_STORE_AGGREGATED_COUNTERSAGGREGATESTORE_H_
#define SRC_STORE_AGGREGATED_COUNTERSAGGREGATESTORE_H_

#include "mpi.h"
#include "aggregatestorebase.h"

namespace meric {

template <class TStore>
class CountersAggregateStore: public AggregateStoreBase<TStore> {

public:
//	CountersAggregateStore(const std::string wrapper, MPI_Comm &comm, bool write): AggregateStoreBase<TStore>(wrapper, comm, write)
	CountersAggregateStore(const std::string wrapper, MPI_Comm &comm): AggregateStoreBase<TStore>(wrapper, comm)
	{
		_bufferCounters.resize(this->_size);
		_bufferTime.resize(this->_size);
	};


protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void saveInterval(std::vector<size_t> &iStarts, size_t iEnd);
	
	std::vector <long>   _bufferCounters;
	std::vector <double> _bufferTime;
	MPI_Datatype consolidate;
};

}

#include "countersaggregatestore.hpp"




#endif /* SRC_STORE_AGGREGATED_COUNTERSAGGREGATESTORE_H_ */
