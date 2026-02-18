
#ifndef SRC_STORE_AGGREGATED_SAMPLESAGGREGATESTORE_H_
#define SRC_STORE_AGGREGATED_SAMPLESAGGREGATESTORE_H_

#include <mpi.h>
#include "aggregatestorebase.h"

namespace meric {

template <class TStore>
class SamplesAggregateStore: public AggregateStoreBase<TStore> {

public:
	typedef typename TStore::samplesType StoreSamplesType;
//	typedef TSystem samplesType;

	SamplesAggregateStore(MPI_Comm &comm, bool write): AggregateStoreBase<TStore>(comm, write)
	{
		MPI_Type_contiguous(3, MPI_DOUBLE, &m_valueType);
		MPI_Type_commit(&m_valueType);

		MPI_Op_create([](void *pA, void *pB, int *len, MPI_Datatype *pDataType) {
			const Value_t *pIn = reinterpret_cast<Value_t *>(pA);
			Value_t *pInOut = reinterpret_cast<Value_t *>(pB);

			for(int i = 0; i < *len; ++i)
			{
				pInOut[i].value = pInOut[i].value + pIn[i].value;
				pInOut[i].min = std::min(pInOut[i].min, pIn[i].min);
				pInOut[i].max = std::max(pInOut[i].max, pIn[i].max);
			}
		}, true, &m_reduceOp);
	}

	virtual ~SamplesAggregateStore()
	{
		MPI_Type_free(&m_valueType);
		MPI_Op_free(&m_reduceOp);
	}

protected:
	struct Value_t {
		Value_t(double val = 0.0)
			: value(val), min(val), max(val)
		{}

		double value;
		double min;
		double max;
	} __attribute__((packed));

	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void saveInterval(std::vector<size_t> &iStarts, size_t iEnd);

	MPI_Datatype m_valueType;
	MPI_Op m_reduceOp;
};

}

#include "samplesaggregatestore.hpp"


#endif /* SRC_STORE_AGGREGATED_SAMPLESAGGREGATESTORE_H_ */
