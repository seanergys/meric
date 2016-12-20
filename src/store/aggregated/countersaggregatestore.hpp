
#include "countersaggregatestore.h"

namespace meric {

template <class TStore>
void CountersAggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	std::vector<long> stats;
	double time;
	this->_store->fillRegionStats(stats, &time, iStarts, iEnd);
	_bufferCounters.resize(this->_size * stats.size());

//TODO - basic mpi vs aggregate mpi counter store se lisi predevsim v tom, ktere procesy sbiraji a nasledne zapisuji vysledky
	MPI_Gather(stats.data(), stats.size(), MPI_LONG, _bufferCounters.data(), stats.size(), MPI_LONG, 0, this->_comm);
	MPI_Gather(&time, 1, MPI_DOUBLE, &_bufferTime[0], 1, MPI_DOUBLE, 0, this->_comm);

	auto printStats = [&ss](const std::string description, const auto& data, const int dataSize, const int stride)	//lambda C++14 (auto argument)
	{
		auto max = data[0];
		auto min = data[0];
		auto sum = data[0];
		for(int i=stride; i<dataSize; i+=stride)
		{
			if (max < data[i])
				max = data[i];
			else if (min > data[i])
				min = data[i];
			sum += data[i];
		}
		ss << "AVG "<< description << "," << sum/(dataSize/stride) << std::endl
		   << "MAX "<< description << "," << max << std::endl
		   << "MIN "<< description << "," << min << std::endl;
	};

	ss << "# CALLTREE;init_" << this->_store->_initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  this->_store->_name[this->_store->_records[iStarts[j]].region] << "_" << this->_store->_counters[this->_store->_records[iStarts[j]].region];
	}
	ss << std::endl;

	ss << "# Job info - " << this->_store->counterLabel << std::endl
	   << "JOB ID," << this->_store->_jobID << "." << this->_store->_stepID  << std::endl
	   << "Nodelist," << this->_store->_nodelist << std::endl;
	printStats("Runtime of function [s]", _bufferTime, _bufferTime.size(), 1);

	ss << "# COUNTERS - "<<this->_store->counterLabel<<": \n";
	int j = 0;
	for(std::map<std::string, long>::iterator counterIt = this->_store->_records[iEnd].record.begin(); counterIt != this->_store->_records[iEnd].record.end(); ++counterIt)
	{
		printStats(counterIt->first, &_bufferCounters[j++],this->_size*stats.size(),stats.size());
	}
}

template <class TStore>
void CountersAggregateStore<TStore>::saveInterval(std::vector<size_t> &iStarts, size_t iEnd)
{
	std::vector<long> stats;
	double time;
	this->_store->fillRegionStats(stats, &time, iStarts, iEnd);

	MPI_Gather(stats.data(), stats.size(), MPI_LONG, _bufferCounters.data(), stats.size(), MPI_LONG, 0, this->_comm);
	MPI_Gather(&time, 1, MPI_DOUBLE, &_bufferTime[0], 1, MPI_DOUBLE, 0, this->_comm);
}
	
}
