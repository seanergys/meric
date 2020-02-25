
#include "aggregatestore.h"

namespace meric {

template <class TStore>
void AggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	RegionStats stats;
	this->_store->fillRegionStats(stats, iStarts.back(), iEnd);

	//TODO we send the whole structure despite we use just the information about energy
	MPI_Gather(&stats, sizeof(RegionStats), MPI_BYTE, _buffer.data(), sizeof(RegionStats), MPI_BYTE, 0, this->_comm);

	ss << "# CALLTREE;init_" << this->_store->_initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++)
	{
		ss << ";" <<  this->_store->_name[this->_store->_records[iStarts[j]].region] << "_" << this->_store->_counters[this->_store->_records[iStarts[j]].region];
	}

	double sumEnergy = 0,
		   maxEnergy = 0,
		   minEnergy = _buffer[0].energy;

	for (size_t i = 0; i < this->_size; i++)
	{
		if(maxEnergy < _buffer[i].energy)
		{
			maxEnergy = _buffer[i].energy;
		} else if (minEnergy > _buffer[i].energy)
		{
			minEnergy = _buffer[i].energy;
		}

		sumEnergy += _buffer[i].energy;
	}

	ss << "\n# SAMPLES - " << StoreSamplesType::getSourceLabel() << ":\n"
	   << "Average energy consumption [J]," << sumEnergy/this->_size << '\n'
	   << "Maximal energy consumption [J]," << maxEnergy << '\n'
	   << "Minimal energy consumption [J]," << minEnergy << '\n'
	   << "Summary energy consumption [J]," << sumEnergy << '\n';	   
}

template <class TStore>
void AggregateStore<TStore>::saveInterval(std::vector<size_t> &iStarts, size_t iEnd)
{
	RegionStats stats;
	this->_store->fillRegionStats(stats, iStarts.back(), iEnd);

	MPI_Gather(&stats, sizeof(RegionStats), MPI_BYTE, _buffer.data(), sizeof(RegionStats), MPI_BYTE, 0, this->_comm);
}

}


