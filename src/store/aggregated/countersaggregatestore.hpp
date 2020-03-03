
#include "countersaggregatestore.h"
#include "../countersstore.h"

namespace meric {

template <class TStore>
void CountersAggregateStore<TStore>::printStats (const std::string description, const StoreDataType * data, const int dataSize, const int stride, bool storeSum, std::ofstream &ss, StoreDataType &avg)
{
	StoreDataType max = data[0];
	StoreDataType min = data[0];
	StoreDataType sum = data[0];
	for(int i=stride; i<dataSize; i+=stride)
	{
		if (max < data[i])
			max = data[i];
		else if (min > data[i])
			min = data[i];
		sum += data[i];
	}
	//StoreDataType 
	avg = sum/(dataSize/stride);
	ss << "AVG "<< description << "," << avg << std::endl
	   << "MAX "<< description << "," << max << std::endl
	   << "MIN "<< description << "," << min << std::endl;
	if (storeSum)
		ss << "SUM "<< description << "," << sum << std::endl;
}

template <class TStore>
void CountersAggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	std::vector<StoreDataType> stats;
	double time;
	this->_store->fillRegionStats(stats, &time, iStarts.back(), iEnd);
	_bufferCounters.resize(this->_size * stats.size());

//INFO - basic mpi vs aggregate mpi counter store se lisi predevsim v tom, ktere procesy sbiraji a nasledne zapisuji vysledky
	MPI_Gather(stats.data(), stats.size(), MPI_UNSIGNED_LONG_LONG, _bufferCounters.data(), stats.size(), MPI_UNSIGNED_LONG_LONG, 0, this->_comm);
	MPI_Gather(&time, 1, MPI_DOUBLE, &_bufferTime[0], 1, MPI_DOUBLE, 0, this->_comm);
	
/*
	auto printStats = [&ss](const std::string description, const auto& data, const int dataSize, const int stride, bool storeSum)	//lambda C++14 (auto argument)
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
		auto avg = sum/(dataSize/stride);
		ss << "AVG "<< description << "," << avg << std::endl
		   << "MAX "<< description << "," << max << std::endl
		   << "MIN "<< description << "," << min << std::endl;
		if (storeSum)
			ss << "SUM "<< description << "," << sum << std::endl;
		return avg;
	};
*/
	ss << "# CALLTREE;init_" << this->_store->_initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++)
	{
		ss << ";" <<  this->_store->_name[this->_store->_records[iStarts[j]].region] << "_" << this->_store->_counters[this->_store->_records[iStarts[j]].region];
	}

	ss << "\n# COUNTERS - "<<StoreCounterType::getSourceLabel() << ":\n";
	int j = 0;
	CountersRecord<StoreDataType> averageRecord;
	//for(std::map<std::string, StoreDataType>::iterator counterIt = this->_store->_records[iEnd].record.begin(); ...)
	for(auto counterIt = this->_store->_records[iEnd].record.begin(); counterIt != this->_store->_records[iEnd].record.end(); ++counterIt)
	{
		//averageRecord.record[counterIt->first] = printStats(counterIt->first, &_bufferCounters[j++],this->_size*stats.size(),stats.size(), true);
		printStats(counterIt->first, & _bufferCounters[j++],this->_size*stats.size(),stats.size(), true, ss, averageRecord.record[counterIt->first]);
	}

// Counters ratios
	CountersRecord<StoreDataType> startRecord;
	std::map <std::string, double> ratios;
	
	double sumTime = 0.0;
	for(double nodeTime : _bufferTime)
	{
		sumTime += nodeTime;
	}
	time = sumTime/_bufferTime.size();
	
	StoreCounterType::getRatios(startRecord.record, averageRecord.record, time, ratios);
	if (ratios.size())
	{
		ss << "# RATIOS:\n";
		for(auto const &element : ratios)
			ss << "AVG " << StoreCounterType::getCounterLabel(element.first) <<"," << std::fixed << std::setprecision(5) << element.second <<std::endl;
	}
}

template <class TStore>
void CountersAggregateStore<TStore>::saveInterval(std::vector<size_t> &iStarts, size_t iEnd)
{
	std::vector<StoreDataType> stats;
	double time;
	this->_store->fillRegionStats(stats, &time, iStarts.back(), iEnd);

	MPI_Gather(stats.data(), stats.size(), MPI_UNSIGNED_LONG_LONG, _bufferCounters.data(), stats.size(), MPI_UNSIGNED_LONG_LONG, 0, this->_comm);
	MPI_Gather(&time, 1, MPI_DOUBLE, &_bufferTime[0], 1, MPI_DOUBLE, 0, this->_comm);
}
	
}
