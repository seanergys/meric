
#include "infoaggregatestore.h"

namespace meric {

template <class TStore>
void InfoAggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	InfoStats stats;
	this->_store->fillRegionStats(stats, iStarts.back(), iEnd);
	MPI_Gather(&stats, sizeof(InfoStats), MPI_BYTE, _buffer.data(), sizeof(InfoStats), MPI_BYTE, 0, this->_comm);

	ss << "# CALLTREE;init_" << this->_store->_initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++)
	{
		ss << ";" <<  this->_store->_name[this->_store->_records[iStarts[j]].region] << "_" << this->_store->_counters[this->_store->_records[iStarts[j]].region];
	}

	ss << "\n# Job info" << std::endl
	   << "JOB ID," << this->_store->_jobID << "." << this->_store->_stepID  << std::endl
	   << "Nodelist," << this->_store->_nodelist << std::endl;

	double sumDuration = 0,
		   maxDuration = 0,
		   minDuration = _buffer[0].duration;

	for (size_t i = 0; i < this->_size; i++)
	{
		if(maxDuration < _buffer[i].duration)
		{
			maxDuration = _buffer[i].duration;
		} else if (minDuration > _buffer[i].duration)
		{
			minDuration = _buffer[i].duration;
		}
		sumDuration += _buffer[i].duration;
	}
	ss << "AVG Runtime of function [s]," << sumDuration/this->_size << std::endl
	   << "MAX Runtime of function [s]," << maxDuration << std::endl
	   << "MIN Runtime of function [s]," << minDuration << std::endl;

	double sumStart = 0,
		   maxStart = 0,
		   minStart = _buffer[0].startTime;

	for (size_t i = 0; i < this->_size; i++)
	{
		if(maxStart < _buffer[i].startTime)
		{
			maxStart = _buffer[i].startTime;
		} else if (minStart > _buffer[i].startTime)
		{
			minStart = _buffer[i].startTime;
		}
		sumStart += _buffer[i].startTime;
	}
	ss << "AVG Function start timestamp," << std::setprecision (15) << sumStart/this->_size << std::endl
	   << "MAX Function start timestamp," << maxStart << std::endl
	   << "MIN Function start timestamp," << minStart << std::endl;	
}

template <class TStore>
void InfoAggregateStore<TStore>::saveInterval(std::vector<size_t> &iStarts, size_t iEnd)
{
	InfoStats stats;
	this->_store->fillRegionStats(stats, iStarts.back(), iEnd);
	MPI_Gather(&stats, sizeof(InfoStats), MPI_BYTE, _buffer.data(), sizeof(InfoStats), MPI_BYTE, 0, this->_comm);
}

}


