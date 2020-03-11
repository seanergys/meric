
#include "samplesstore.h"
#include <cmath>

using namespace meric;

template <class TSystem> void SamplesStore<TSystem>::addStart(int region, RegionParameters configuration)
{
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	_records.push_back(SamplesRecord(region, time, configuration));
}

template <class TSystem> void SamplesStore<TSystem>::addEnd(int region)
{
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	_records.push_back(SamplesRecord(region, time, _records.back().configuration));
}

template <class TSystem> void SamplesStore<TSystem>::computePeriods()
{
	TSystem::setTimeAdjustment(_start);
}

template <class TSystem> void SamplesStore<TSystem>::fillRegionStats(DetailedRegionStats &stats, size_t iStarts, size_t iEnd)
{
	std::string msg = "";
	size_t firstBlade, lastBlade;
	size_t firstVR, lastVR;
	double * energy;
	double * min;
	double * max;
	const double duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	energy = new double [TSystem::PROBE_NUMBER];
	min = new double [TSystem::PROBE_NUMBER];
	max = new double [TSystem::PROBE_NUMBER];
	msg = TSystem::getData(mutils::timeToDouble(_records[iStarts].time), duration, &firstBlade, &lastBlade, &firstVR, &lastVR, energy, min, max);

	if (msg != "")
		std::cerr << msg <<  _name[_records[iStarts].region]<< std::endl;
	
	// Blade values
	if(lastBlade > 0 && lastBlade > firstBlade)
	{
		stats.energy[0] = duration * energy[0] / (lastBlade - firstBlade);
	}
	else
		stats.energy[0] = 0;
	
	// Voltage Regualtors values
	if(lastVR > 0 && lastVR > firstVR)
	{
		for (size_t it = 1; it < TSystem::vrSensorLabel.size(); it++)
		{
			stats.energy[it] = duration * energy[it] / (lastVR - firstVR);
		}
	}
	else
		for(size_t i=1; i < HDEEM::PROBE_NUMBER; i++) //DetailedRegionStats.energy size is always HDEEM::PROBE_NUMBER
			stats.energy[i] = 0;
	
	delete [] energy;
	delete [] min;
	delete [] max;
}

template <class TSystem> void SamplesStore<TSystem>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	timespec time1, time2;
	clock_gettime(CLOCK_REALTIME, &time1);
		
    ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	size_t firstBlade, lastBlade;
	size_t firstVR, lastVR;
	double * energy;
	double * minWatts;
	double * maxWatts;
    double avgWatts = 0.0;
    double energyConsumption = 0.0;

	std::string msg = "";
	double duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts.back()].time);
	energy = new double [TSystem::PROBE_NUMBER];
	minWatts = new double [TSystem::PROBE_NUMBER];
	maxWatts = new double [TSystem::PROBE_NUMBER];
	msg = TSystem::getData(mutils::timeToDouble(_records[iStarts.back()].time), duration, &firstBlade, &lastBlade, &firstVR, &lastVR, energy, minWatts, maxWatts);
	
	if (msg != "" && msg.find("detailed") == std::string::npos)
		std::cerr << msg <<  _name[_records[iStarts.back()].region]<< std::endl;

	MERIC_INFO << _name[_records[iStarts.back()].region] << " FIRST: "<< firstBlade << " in time "<< int(mutils::timeToDouble(_records[iStarts.back()].time)) << std::endl;
	MERIC_INFO << _name[_records[iStarts.back()].region] << " LAST:  "<< lastBlade << " in time "<< int(mutils::timeToDouble(_records[iEnd].time))  << std::endl;

    // Blade values
	ss << "# SAMPLES - " << TSystem::getSourceLabel() << ":\n";
	if(lastBlade > 0 && lastBlade > firstBlade)
	{
		if (printSamples)
		{
			ss << "# Blade (" << TSystem::getSourceLabel() <<") samples [W]"<< std::endl;
			std::vector<double> samples;
			TSystem::getSamplesList(0, firstBlade, lastBlade, samples);
			size_t it = firstBlade;
			for (size_t i = 0; i < samples.size(); i++)
				ss << it++ << "," << samples[i] << std::endl;
		}
		avgWatts = energy[0]/(lastBlade-firstBlade);
		energyConsumption = duration * avgWatts;

		ss << "# Blade summary" << std::endl
		   << "Energy consumption [J]" << "," << energyConsumption << std::endl
		   << "Maximal power [W]" << "," << maxWatts[0] << std::endl
		   << "Minimal power [W]" << "," << minWatts[0] << std::endl
		   << "Average power [W]" << "," << avgWatts << std::endl
		   << "ID of first blade sample" << "," << firstBlade << std::endl
		   << "ID of last blade sample" << "," << lastBlade-1 << std::endl;
	} else {
		ss << "# Blade summary - ERROR" << std::endl
		   << "Energy consumption [J],0"<< std::endl;
	}
	
	// Voltage Regulators values
	if(lastVR > 0 && lastVR > firstVR)
	{
		for (unsigned int sensorID = 1; sensorID < TSystem::vrSensorLabel.size(); sensorID++)
		{
			if(printSamples)
			{
				ss << "# Voltage regulator (" << TSystem::vrSensorLabel[sensorID] << ") samples [W]" << std::endl;
				std::vector<double> samples;
				TSystem::getSamplesList(sensorID, firstVR, lastVR, samples);
				size_t it = firstVR;
				for (size_t i = 0; i < samples.size(); i++)
					ss << it++ << "," << samples[i] << std::endl;

			}
	        avgWatts = energy[sensorID]/(lastVR-firstVR);
	        energyConsumption = duration * avgWatts;

		    ss << "# Voltage regulator (" << TSystem::vrSensorLabel[sensorID] << ") summary" <<std::endl
		       << "Energy consumption [J]" << "," << energyConsumption << std::endl
		       << "Maximal power [W]" << "," << maxWatts[sensorID] << std::endl
		       << "Minimal power [W]" << "," << minWatts[sensorID] << std::endl
		       << "Average power [W]" << "," << avgWatts << std::endl
		       << "ID of first VR sample" << "," << firstVR << std::endl
		       << "ID of last VR sample" << "," << lastVR-1 << std::endl;
		}
	}

	delete [] energy;
	delete [] minWatts;
	delete [] maxWatts;
	
	clock_gettime(CLOCK_REALTIME, &time2);
	MERIC_INFO << "MERIC record saved in: " << mutils::timeToDouble(time2 - time1) <<" s\n";
}

template class SamplesStore <HDEEM>;
template class SamplesStore <JETSON>;
template class SamplesStore <THUNDER>;
template class SamplesStore <DAVIDE>;
