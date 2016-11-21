
#ifndef SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_
#define SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_

#include "mpi.h"

#include "../meric.h"

#include "../../store/aggregated/aggregatestore.h"
#include "../../store/aggregated/detailedaggregatestore.h"
#include "../../store/aggregated/debugaggregatestore.h"

#include "../../wrapper/environmentwrapper.h"

namespace meric {

class DistributedMeric: public Meric {

public:
	DistributedMeric();
	~DistributedMeric();

	void start(const std::string &region)
	{
		MPI_Barrier(_currentNode);
		if (!_MPInodeRank) {
			Environment::frequency(regionValue(region).frequency);
			Environment::uncoreFrequency(regionValue(region).uncoreFrequency);
		}
		Environment::threadNumber(regionValue(region).nthreads);
		Meric::start(region);
	}
	
	void stop()
	{
		MPI_Barrier(_currentNode);
		Meric::stop();
		if (!_MPInodeRank) {
			Environment::frequency(regionValue(_store->region()).frequency);
			Environment::uncoreFrequency(regionValue(_store->region()).uncoreFrequency);
		}
		Environment::threadNumber(regionValue(_store->region()).nthreads);
	}
	
	void save()
	{
		MPI_Barrier(_currentNode);
		if (!_MPInodeRank) {
			if (Meric::AGGREGATE) {
				_store->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _nodeName);
			} else {
				_store->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _nodeName);
			}
		}
		sleep(1);
		MPI_Barrier(_currentNode);
	}

protected:
	int _MPIglobalRank;
	int _MPIglobalSize;
	int _MPInodeRank;
	int _MPInodeSize;
	std::string _nodeName;
	MPI_Comm _currentNode;
	MPI_Comm _storingProcs;
};

}

#endif /* SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_ */
