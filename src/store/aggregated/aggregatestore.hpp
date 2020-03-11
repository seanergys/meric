
#include "aggregatestore.h"

namespace meric {

template <class TStore>
void AggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	DetailedRegionStats stats;
	this->_store->fillRegionStats(stats, iStarts.back(), iEnd);
	MPI_Gather(&stats, sizeof(DetailedRegionStats), MPI_BYTE, _buffer.data(), sizeof(DetailedRegionStats), MPI_BYTE, 0, this->_comm);

	ss << "# CALLTREE;init_" << this->_store->_initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++)
	{
		ss << ";" <<  this->_store->_name[this->_store->_records[iStarts[j]].region] << "_" << this->_store->_counters[this->_store->_records[iStarts[j]].region];
	}

	double sumEnergy = 0,
		   maxEnergy = 0,
		   minEnergy = _buffer[0].energy[0];

	for (size_t i = 0; i < this->_size; i++)
	{
		if(maxEnergy < _buffer[i].energy[0])
		{
			maxEnergy = _buffer[i].energy[0];
		} else if (minEnergy > _buffer[i].energy[0])
		{
			minEnergy = _buffer[i].energy[0];
		}
		sumEnergy += _buffer[i].energy[0];
	}

	if (sumEnergy > 0.0)
	{
		ss << "\n# SAMPLES - " << StoreSamplesType::getSourceLabel() << ":" << std::endl
		   << "AVG energy consumption [J]," << sumEnergy/this->_size << std::endl
		   << "MAX energy consumption [J]," << maxEnergy << std::endl
		   << "MIN energy consumption [J]," << minEnergy << std::endl
		   << "SUM energy consumption [J]," << sumEnergy << std::endl;
	}
	else
	{
		ss << "\n# SAMPLES - " << StoreSamplesType::getSourceLabel() << ":" << std::endl
		   << "# Blade summary - ERROR" << std::endl
		   << "SUM energy consumption [J],0"<< std::endl;
	}

	for (unsigned int sensorID = 1; sensorID < StoreSamplesType::vrSensorLabel.size(); sensorID++)
	{
		sumEnergy = 0;
		maxEnergy = 0;
		minEnergy = _buffer[0].energy[sensorID];

		// Iterate through nodes
		for (size_t i = 0; i < this->_size; i++)
		{
			if(maxEnergy < _buffer[i].energy[sensorID])
			{
				maxEnergy = _buffer[i].energy[sensorID];
			} else if (minEnergy > _buffer[i].energy[sensorID]) 
			{
				minEnergy = _buffer[i].energy[sensorID];
			}
			sumEnergy += _buffer[i].energy[sensorID];
		}

		if (sumEnergy > 0.0)
			ss << "# Voltage regulator (" << StoreSamplesType::vrSensorLabel[sensorID] << ") - " << StoreSamplesType::getSourceLabel() << ":" << std::endl
			   << "AVG energy consumption [J]," << sumEnergy/this->_size << std::endl
			   << "MAX energy consumption [J]," << maxEnergy << std::endl
			   << "MIN energy consumption [J]," << minEnergy << std::endl
			   << "SUM energy consumption [J]," << sumEnergy << std::endl;
	}
}

template <class TStore>
void AggregateStore<TStore>::saveInterval(std::vector<size_t> &iStarts, size_t iEnd)
{
	DetailedRegionStats stats;
	this->_store->fillRegionStats(stats, iStarts.back(), iEnd);

	//TODO in non-detailed mode we should send just the BLADE
	MPI_Gather(&stats, sizeof(DetailedRegionStats), MPI_BYTE, _buffer.data(), sizeof(DetailedRegionStats), MPI_BYTE, 0, this->_comm);
}

}


