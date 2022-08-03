
#ifndef SRC_STORE_AGGREGATED_COUNTERSAGGREGATESTORE_H_
#define SRC_STORE_AGGREGATED_COUNTERSAGGREGATESTORE_H_

#include <mpi.h>
#include "aggregatestorebase.h"

namespace meric {

template <class TStore>
class CountersAggregateStore: public AggregateStoreBase<TStore> {

public:
	typedef typename TStore::dataType StoreDataType;
	typedef typename TStore::resultType StoreResultType;
	typedef typename TStore::counterType StoreCounterType;
	CountersAggregateStore(MPI_Comm &comm, bool write): AggregateStoreBase<TStore>(comm, write)
	{
		_bufferCounters.resize(this->_size);
		_bufferRatios.resize(this->_size);
	};


protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void saveInterval(std::vector<size_t> &iStarts, size_t iEnd);
	
	template <typename T> void printStats (const std::string description, const T * data, const int dataSize, const int stride, bool storeSum, std::ofstream &ss)
	{
		if (description != "")
		{
			T max = data[0];
			T min = data[0];
			T sum = data[0];
			for(int i=stride; i<dataSize; i+=stride)
			{
				if (max < data[i])
					max = data[i];
				else if (min > data[i])
					min = data[i];
				sum += data[i];
			}
			T avg = sum/(dataSize/stride);
			ss << "AVG "<< description << "," << std::fixed << std::setprecision(6) << avg << std::endl
			   << "MAX "<< description << "," << max << std::endl
			   << "MIN "<< description << "," << min << std::endl;
			if (storeSum)
				ss << "SUM "<< description << "," << sum << std::endl;
		}
	}
	
	std::vector <StoreResultType> _bufferCounters;
	std::vector <double> _bufferRatios;
	MPI_Datatype consolidate;
};

}

#include "countersaggregatestore.hpp"




#endif /* SRC_STORE_AGGREGATED_COUNTERSAGGREGATESTORE_H_ */
