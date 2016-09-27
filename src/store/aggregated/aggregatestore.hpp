
#include "aggregatestore.h"

namespace meric {

template <class TStore>
void AggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	RegionStats stats;
	_store.fillRegionStats(stats, iStarts.back(), iEnd);

	MPI_Gather(&stats, sizeof(RegionStats), MPI_BYTE, _buffer.data(), sizeof(RegionStats), MPI_BYTE, 0, _comm);

	ss << "aggregated: ";
	for (size_t i = 0; i < _size; i++) {
		ss << _buffer[i].energy << " (" << _buffer[i].duration << "s) ";
	}
	ss << "\n";
}

template <class TStore>
void AggregateStore<TStore>::saveInterval(std::vector<size_t> &iStarts, size_t iEnd)
{
	RegionStats stats;
	_store.fillRegionStats(stats, iStarts.back(), iEnd);

	MPI_Gather(&stats, sizeof(RegionStats), MPI_BYTE, _buffer.data(), sizeof(RegionStats), MPI_BYTE, 0, _comm);
}

}


