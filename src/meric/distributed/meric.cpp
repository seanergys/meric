
#include "meric.h"

using namespace meric;

DistributedMeric::DistributedMeric()
{
	MPI_Comm_rank(MPI_COMM_WORLD, &_MPIrank);
	MPI_Comm_size(MPI_COMM_WORLD, &_MPIsize);
}

void DistributedMeric::start(const std::string &region)
{

}

void DistributedMeric::stop()
{

}




