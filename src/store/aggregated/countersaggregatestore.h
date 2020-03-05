
#ifndef SRC_STORE_AGGREGATED_COUNTERSAGGREGATESTORE_H_
#define SRC_STORE_AGGREGATED_COUNTERSAGGREGATESTORE_H_

#include <mpi.h>
#include "aggregatestorebase.h"

namespace meric {

template <class TStore>
class CountersAggregateStore: public AggregateStoreBase<TStore> {

public:
	typedef typename TStore::dataType StoreDataType;
	typedef typename TStore::counterType StoreCounterType;
	CountersAggregateStore(MPI_Comm &comm, bool write): AggregateStoreBase<TStore>(comm, write)
	{
		_bufferCounters.resize(this->_size);
		_bufferTime.resize(this->_size);
	};


protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void saveInterval(std::vector<size_t> &iStarts, size_t iEnd);
	void printStats (const std::string description, const StoreDataType * data, const int dataSize, const int stride, bool storeSum, std::ofstream &ss, StoreDataType &avg);
	
	std::vector <StoreDataType>   _bufferCounters;
	std::vector <double> _bufferTime;
	MPI_Datatype consolidate;
};

}

#include "countersaggregatestore.hpp"




#endif /* SRC_STORE_AGGREGATED_COUNTERSAGGREGATESTORE_H_ */
