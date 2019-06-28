
#include "detailedaggregatestore.h"

namespace meric {

template <class TStore>
void DetailedAggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
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

	//////////////////
	// Blade values //
	//////////////////

	ss << "\n# Blade summary\n"
	   << "Average energy consumption [J]," << sumEnergy/this->_size << '\n'
	   << "Maximal energy consumption [J]," << maxEnergy << '\n'
	   << "Minimal energy consumption [J]," << minEnergy << '\n'
	   << "Summary energy consumption [J]," << sumEnergy << '\n';

	///////////////////////
	// VR sensors values //
	///////////////////////

	std::vector<std::string> vrSensor = {"CPU0", "CPU1", "DDR AB", "DDR CD", "DDR EF", "DDR GH"};
	unsigned int vrSensorNum = vrSensor.size()+1;

	for (unsigned int sensorID = 1; sensorID < vrSensorNum; sensorID++)
	{
		ss << "# Voltage regulator (" << vrSensor[sensorID-1] << ") summary - HDEEM\n";

		sumEnergy = 0;
		maxEnergy = 0;
		minEnergy = _buffer[sensorID].energy[sensorID];

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

		ss << "Average energy consumption [J]," << sumEnergy/this->_size << '\n'
		<< "Maximal energy consumption [J]," << maxEnergy << '\n'
		<< "Minimal energy consumption [J]," << minEnergy << '\n'
		<< "Summary energy consumption [J]," << sumEnergy << '\n';
	}
}

template <class TStore>
void DetailedAggregateStore<TStore>::saveInterval(std::vector<size_t> &iStarts, size_t iEnd)
{
	DetailedRegionStats stats;
	this->_store->fillRegionStats(stats, iStarts.back(), iEnd);

	MPI_Gather(&stats, sizeof(DetailedRegionStats), MPI_BYTE, _buffer.data(), sizeof(DetailedRegionStats), MPI_BYTE, 0, this->_comm);
}


}
