
#include "detailedaggregatestore.h"

namespace meric {

template <class TStore>
void DetailedAggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	DetailedRegionStats stats;
	this->_store.fillRegionStats(stats, iStarts.back(), iEnd);

	MPI_Gather(&stats, sizeof(DetailedRegionStats), MPI_BYTE, _buffer.data(), sizeof(DetailedRegionStats), MPI_BYTE, 0, this->_comm);

	std::cout << this->_store._initCounter << "\n";
	std::cout << this->_store._jobID << "\n";
	std::cout << "regions: " << this->_store._regions.size() << "\n";
	std::cout << "name: " << this->_store._name.size() << "\n";
	std::cout << "counters: " << this->_store._counters.size() << "\n";
	ss << "detailed aggregated: ";
	for (size_t i = 0; i < this->_size; i++) {
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
	this->_store.fillRegionStats(stats, iStarts.back(), iEnd);

	MPI_Gather(&stats, sizeof(DetailedRegionStats), MPI_BYTE, _buffer.data(), sizeof(DetailedRegionStats), MPI_BYTE, 0, this->_comm);
}


}
