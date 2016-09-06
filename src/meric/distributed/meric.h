
#ifndef SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_
#define SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_

#include "mpi.h"

#include "../meric.h"

namespace meric {

class DistributedMeric: public Meric {

public:
	DistributedMeric();

	void start(const std::string &region);
	void stop();

protected:
	int _MPIrank;
	int _MPIsize;

};

}

#endif /* SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_ */
