
#ifndef SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_
#define SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_

#include "mpi.h"

#include "../meric.h"

namespace meric {

class DistributedMeric: public Meric {

public:
	DistributedMeric();

	void start(const std::string &region)
	{
		MPI_Barrier(_currentNode);
		Meric::start(region);
	}
	void stop()
	{
		MPI_Barrier(_currentNode);
		Meric::stop();
	}

protected:
	int _MPIglobalRank;
	int _MPIglobalSize;
	int _MPInodeRank;
	int _MPInodeSize;
	MPI_Comm _currentNode;

};

}

#endif /* SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_ */
