
#ifndef SRC_STORE_AGGREGATED_COUNTERSAGGREGATESTORE_H_
#define SRC_STORE_AGGREGATED_COUNTERSAGGREGATESTORE_H_

#include "mpi.h"
#include "aggregatestorebase.h"

namespace meric {

template <class TStore>
class CountersAggregateStore: public AggregateStoreBase<TStore> {

public:
//	CountersAggregateStore(const std::string wrapper, MPI_Comm &comm, bool write): AggregateStoreBase<TStore>(wrapper, comm, write)
	CountersAggregateStore(MPI_Comm &comm): AggregateStoreBase<TStore>(comm)
	{
		_bufferCounters.resize(this->_size);
		_bufferTime.resize(this->_size);
	};


protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void saveInterval(std::vector<size_t> &iStarts, size_t iEnd);

	double printStats (const std::string description, std::vector<double>& data, const int dataSize, const int stride, bool storeSum, std::ofstream &ss);
	unsigned long long int printStats (const std::string description, const unsigned long long int * data, const int dataSize, const int stride, bool storeSum, std::ofstream &ss);

	
	std::vector <unsigned long long int>   _bufferCounters;
	std::vector <double> _bufferTime;
	MPI_Datatype consolidate;
};

}

#include "countersaggregatestore.hpp"




#endif /* SRC_STORE_AGGREGATED_COUNTERSAGGREGATESTORE_H_ */
