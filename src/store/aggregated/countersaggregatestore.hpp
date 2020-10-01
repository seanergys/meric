
#include "countersaggregatestore.h"
#include "../countersstore.h"

namespace meric {

template <class TStore>
void CountersAggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	std::vector<StoreResultType> stats;
	std::map <std::string, double> ratios;
	this->_store->fillRegionStats(stats, ratios, iStarts.back(), iEnd);
	_bufferCounters.resize(this->_size * stats.size());
	std::vector <double> ratiosList;
	for (const auto &r : ratios)
		ratiosList.push_back(r.second);
	_bufferRatios.resize(this->_size * ratiosList.size());

	MPI_Gather(stats.data(), stats.size()*sizeof(StoreResultType), MPI_BYTE, _bufferCounters.data(), stats.size()*sizeof(StoreResultType), MPI_BYTE, 0, this->_comm);
	MPI_Gather(ratiosList.data(), ratiosList.size(), MPI_DOUBLE, _bufferRatios.data(), ratiosList.size(), MPI_DOUBLE, 0, this->_comm);

	ss << "# CALLTREE;init_" << this->_store->_initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++)
	{
		ss << ";" <<  this->_store->_name[this->_store->_records[iStarts[j]].region] << "_" << this->_store->_counters[this->_store->_records[iStarts[j]].region];
	}

	ss << "\n# COUNTERS - "<<StoreCounterType::getSourceLabel() << ":\n";
	int j = 0;
	CountersRecord<StoreResultType> averageRecord;
	//for(std::map<std::string, StoreResultType>::iterator counterIt = this->_store->_records[iEnd].record.begin(); ...)
	for(auto counterIt = this->_store->_records[iEnd].record.begin(); counterIt != this->_store->_records[iEnd].record.end(); ++counterIt)
	{
		printStats(counterIt->first, & _bufferCounters[j++],this->_size*stats.size(),stats.size(), true, ss);
	}

	if (ratiosList.size())
	{
		ss << "# RATIOS:\n";
		int j=0;
		for(auto const &r : ratios)
			printStats(StoreCounterType::getCounterLabel(r.first), & _bufferRatios[j++],this->_size*ratiosList.size(),ratiosList.size(), false, ss);	
	}
}

template <class TStore>
void CountersAggregateStore<TStore>::saveInterval(std::vector<size_t> &iStarts, size_t iEnd)
{
	std::vector<StoreResultType> stats;
	std::map <std::string, double> ratios;
	this->_store->fillRegionStats(stats, ratios, iStarts.back(), iEnd);
	std::vector <double> ratiosList;
	for (const auto &r : ratios)
		ratiosList.push_back(r.second);

	MPI_Gather(stats.data(), stats.size()*sizeof(StoreResultType), MPI_BYTE, _bufferCounters.data(), stats.size()*sizeof(StoreResultType), MPI_BYTE, 0, this->_comm);
	MPI_Gather(ratiosList.data(), ratiosList.size(), MPI_DOUBLE, _bufferRatios.data(), ratiosList.size(), MPI_DOUBLE, 0, this->_comm);
}
	
}
