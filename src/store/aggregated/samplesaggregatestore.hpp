
#include "samplesaggregatestore.h"

namespace meric {

template <class TStore>
void SamplesAggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	DetailedRegionStats stats;
	this->_store->fillRegionStats(stats, iStarts.back(), iEnd);

	Value_t totalEnergyIn(stats.totalEnergy), totalEnergyOut;

	std::vector<Value_t> energyIn, energyOut;
	for(const auto &value: stats.energy)
	{
		energyIn.push_back(Value_t(value));
	}

	energyOut.resize(energyIn.size());

	MPI_Reduce(&totalEnergyIn, &totalEnergyOut, 1, m_valueType, m_reduceOp, 0, this->_comm);
	MPI_Reduce(energyIn.data(), energyOut.data(), energyIn.size(), m_valueType, m_reduceOp, 0, this->_comm);

	ss << "# CALLTREE;init_" << this->_store->_initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++)
	{
		ss << ";" <<  this->_store->_name[this->_store->_records[iStarts[j]].region] << "_" << this->_store->_counters[this->_store->_records[iStarts[j]].region];
	}

	if(totalEnergyOut.value > 0.0)
	{
		ss << "\n# SAMPLES - " << StoreSamplesType::getSourceLabel() << " (Total):\n"
		   << "AVG Energy consumption [J]," << std::fixed << std::setprecision (6) << totalEnergyOut.value / this->_size << '\n'
		   << "MAX Energy consumption [J]," << totalEnergyOut.max << '\n'
		   << "MIN Energy consumption [J]," << totalEnergyOut.min << '\n'
		   << "SUM Energy consumption [J]," << totalEnergyOut.value << '\n';
	}
	else
	{
		ss << "\n# SAMPLES - " << StoreSamplesType::getSourceLabel() << " (Total):\n"
		   << "# Total summary - ERROR\n"
		   << "SUM Energy consumption [J],0\n";
	}

	for(size_t i = 0; i < energyOut.size(); ++i)
	{
		if(energyOut[i].value <= 0.0)
			continue;

		ss << "# SAMPLES - " << StoreSamplesType::getSourceLabel() << " (" << StoreSamplesType::getSamplerLabel(i) << "):\n"
			<< "AVG Energy consumption [J]," << std::fixed << std::setprecision (6) << energyOut[i].value / this->_size << '\n'
			<< "MAX Energy consumption [J]," << energyOut[i].max << '\n'
			<< "MIN Energy consumption [J]," << energyOut[i].min << '\n'
			<< "SUM Energy consumption [J]," << energyOut[i].value << '\n';
	}
}

template <class TStore>
void SamplesAggregateStore<TStore>::saveInterval(std::vector<size_t> &iStarts, size_t iEnd)
{
	DetailedRegionStats stats;
	this->_store->fillRegionStats(stats, iStarts.back(), iEnd);

	Value_t totalEnergyIn(stats.totalEnergy), totalEnergyOut;

	std::vector<Value_t> energyIn, energyOut;
	for(const auto &value: stats.energy)
	{
		energyIn.push_back(Value_t(value));
	}

	energyOut.resize(energyIn.size());

	MPI_Reduce(&totalEnergyIn, &totalEnergyOut, 1, m_valueType, m_reduceOp, 0, this->_comm);
	MPI_Reduce(energyIn.data(), energyOut.data(), energyIn.size(), m_valueType, m_reduceOp, 0, this->_comm);
}

}


