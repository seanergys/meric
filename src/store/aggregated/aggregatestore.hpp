
#include "aggregatestore.h"

namespace meric {

template <class TStore>
void AggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	RegionStats stats;
	this->_store.fillRegionStats(stats, iStarts.back(), iEnd);

	MPI_Gather(&stats, sizeof(RegionStats), MPI_BYTE, _buffer.data(), sizeof(RegionStats), MPI_BYTE, 0, this->_comm);

	std::cout << this->_store._initCounter << "\n";
	std::cout << this->_store._jobID << "\n";
	std::cout << "regions: " << this->_store._regions.size() << "\n";
	std::cout << "name: " << this->_store._name.size() << "\n";
	std::cout << "counters: " << this->_store._counters.size() << "\n";
	ss << "aggregated: ";
	for (size_t i = 0; i < this->_size; i++) {
		ss << _buffer[i].energy << " (" << _buffer[i].duration << "s) ";
	}
	ss << "\n";
}

template <class TStore>
void AggregateStore<TStore>::saveInterval(std::vector<size_t> &iStarts, size_t iEnd)
{
	RegionStats stats;
	this->_store.fillRegionStats(stats, iStarts.back(), iEnd);

	MPI_Gather(&stats, sizeof(RegionStats), MPI_BYTE, _buffer.data(), sizeof(RegionStats), MPI_BYTE, 0, this->_comm);
}

}


