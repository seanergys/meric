
#include "samplesaggregatestore.h"

namespace meric {

template <class TStore>
void SamplesAggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
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
		ss << "\n# SAMPLES - " << StoreSamplesType::getSourceLabel() << " (Blade):\n"
		   << "AVG Energy consumption [J]," << std::fixed << std::setprecision (6) << sumEnergy/this->_size << '\n'
		   << "MAX Energy consumption [J]," << maxEnergy << '\n'
		   << "MIN Energy consumption [J]," << minEnergy << '\n'
		   << "SUM Energy consumption [J]," << sumEnergy << '\n';
	}
	else
	{
		ss << "\n# SAMPLES - " << StoreSamplesType::getSourceLabel() << " (Blade):\n"
		   << "# Blade summary - ERROR\n"
		   << "SUM Energy consumption [J],0\n";
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
			ss << "# SAMPLES - " << StoreSamplesType::getSourceLabel() << " ("<< StoreSamplesType::vrSensorLabel[sensorID] <<"):\n"
			   << "AVG Energy consumption [J]," << std::fixed << std::setprecision (6) << sumEnergy/this->_size << '\n'
			   << "MAX Energy consumption [J]," << maxEnergy << '\n'
			   << "MIN Energy consumption [J]," << minEnergy << '\n'
			   << "SUM Energy consumption [J]," << sumEnergy << '\n';
	}
}

template <class TStore>
void SamplesAggregateStore<TStore>::saveInterval(std::vector<size_t> &iStarts, size_t iEnd)
{
	DetailedRegionStats stats;
	this->_store->fillRegionStats(stats, iStarts.back(), iEnd);

	//TODO in non-detailed mode we should send just the BLADE
	MPI_Gather(&stats, sizeof(DetailedRegionStats), MPI_BYTE, _buffer.data(), sizeof(DetailedRegionStats), MPI_BYTE, 0, this->_comm);
}

}


