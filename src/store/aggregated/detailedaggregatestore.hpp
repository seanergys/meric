
#include "detailedaggregatestore.h"

namespace meric {

template <class TStore>
void DetailedAggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	DetailedRegionStats stats;
	_store.fillRegionStats(stats, iStarts.back(), iEnd);

	MPI_Gather(&stats, sizeof(DetailedRegionStats), MPI_BYTE, _buffer.data(), sizeof(DetailedRegionStats), MPI_BYTE, 0, _comm);

	std::cout << _store._initCounter << "\n";
	std::cout << _store._jobID << "\n";
	std::cout << "regions: " << _store._regions.size() << "\n";
	std::cout << "name: " << _store._name.size() << "\n";
	std::cout << "counters: " << _store._counters.size() << "\n";
	ss << "detailed aggregated: ";
	for (size_t i = 0; i < _size; i++) {
		for (size_t j = 0; j < PROBE_NUMBER; j++) {
			ss << _buffer[i].energy[j] << " ";
		}
		ss << " (" << _buffer[i].duration << "s) ";
	}
	ss << "\n";
}

template <class TStore>
void DetailedAggregateStore<TStore>::saveInterval(std::vector<size_t> &iStarts, size_t iEnd)
{
	DetailedRegionStats stats;
	_store.fillRegionStats(stats, iStarts.back(), iEnd);

	MPI_Gather(&stats, sizeof(DetailedRegionStats), MPI_BYTE, _buffer.data(), sizeof(DetailedRegionStats), MPI_BYTE, 0, _comm);
}


}
