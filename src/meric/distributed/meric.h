
#ifndef SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_
#define SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_

#include "mpi.h"

#include "../meric.h"

#include "../../store/aggregated/aggregatestore.h"
#include "../../store/aggregated/detailedaggregatestore.h"

namespace meric {

class DistributedMeric: public Meric {

public:
	DistributedMeric();
	~DistributedMeric();

	void start(const std::string &region)
	{
		MPI_Barrier(_currentNode);
		if (!_MPInodeRank) {
			Meric::start(region);
		}
	}
	void stop()
	{
		MPI_Barrier(_currentNode);
		if (!_MPInodeRank) {
			Meric::stop();
		}
	}
	void save()
	{
		MPI_Barrier(_currentNode);
		if (!_MPInodeRank) {
			HDEEM::stop();
		}
		if (!_MPInodeRank) {
			if (Meric::AGGREGATE) {
				_store->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _nodeName);
			} else {
				_store->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _nodeName);
			}
		}
		sleep(1);
		MPI_Barrier(_currentNode);
		if (!_MPInodeRank) {
			HDEEM::start();
		}
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
