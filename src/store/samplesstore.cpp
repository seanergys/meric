
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
		std::cerr << msg <<  _name[_records[iStarts].region] << std::endl;
	
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

	MERIC_INFO << _name[_records[iStarts.back()].region] << " FIRST: " << firstBlade << " in time " << int(mutils::timeToDouble(_records[iStarts.back()].time)) << std::endl;
	MERIC_INFO << _name[_records[iStarts.back()].region] << " LAST:  " << lastBlade << " in time " << int(mutils::timeToDouble(_records[iEnd].time))  << std::endl;
	
	bool staticRegion = false;
	std::ofstream samplesStream;
	if (printSamples)
	{
		const size_t staticRegionSufixLength = std::string("_static").length();
		staticRegion = _name[_records[iStarts.back()].region].length() > staticRegionSufixLength && _name[_records[iStarts.back()].region].compare(_name[_records[iStarts.back()].region].length() - staticRegionSufixLength, staticRegionSufixLength, "_static") == 0;
		if (staticRegion)
		{
			const std::string samplesDir = outputDirectory+"/"+_name[_records[iStarts.back()].region]+"/samples/";
			mutils::createDir(samplesDir);			
			samplesStream.open(samplesDir + (outputPrefix.size() ? outputPrefix + "_" : "") + std::to_string(_initCounter) + ".csv", std::ofstream::app);
		}
	}

    // Blade values
	if(lastBlade > 0 && lastBlade > firstBlade)
	{
		if (printSamples && staticRegion)
		{
			samplesStream << "# SAMPLES - " << TSystem::getSourceLabel() << " (Blade) power [W]:\n";
			std::vector<double> samples;
			TSystem::getSamplesList(0, firstBlade, lastBlade, samples);
			size_t it = firstBlade;
			for (size_t i = 0; i < samples.size(); i++)
				samplesStream << it++ << "," << samples[i] << '\n';
		}
		avgWatts = energy[0]/(lastBlade-firstBlade);
		energyConsumption = duration * avgWatts;

		ss << "# SAMPLES - " << TSystem::getSourceLabel() << " (Blade):\n"
		   << TSystem::SUM_ENERGY_LABEL << "," << std::fixed << std::setprecision (6) << energyConsumption << '\n'
		   << "MAX power [W]" << "," << maxWatts[0] << '\n'
		   << "MIN power [W]" << "," << minWatts[0] << '\n'
		   << "AVG power [W]" << "," << avgWatts << '\n'
		   << "ID of first sample" << "," << firstBlade << '\n'
		   << "ID of last sample" << "," << lastBlade-1 << '\n';
	}
	else
	{
		ss << "\n# SAMPLES - " << TSystem::getSourceLabel() << " (Blade):\n"
		   << "# Blade summary - ERROR\n"
		   << TSystem::SUM_ENERGY_LABEL << ",0\n";
	}
	
	// Voltage Regulators values
	if(lastVR > 0 && lastVR > firstVR)
	{
		for (unsigned int sensorID = 1; sensorID < TSystem::vrSensorLabel.size(); sensorID++)
		{
			if(printSamples && staticRegion)
			{
				samplesStream << "# SAMPLES - " << TSystem::getSourceLabel() << " (" << TSystem::vrSensorLabel[sensorID] << ") power [W]:\n";
				std::vector<double> samples;
				TSystem::getSamplesList(sensorID, firstVR, lastVR, samples);
				size_t it = firstVR;
				for (size_t i = 0; i < samples.size(); i++)
					samplesStream << it++ << "," << samples[i] << '\n';

			}
			avgWatts = energy[sensorID]/(lastVR-firstVR);
			energyConsumption = duration * avgWatts;

		    ss << "# SAMPLES - " << TSystem::getSourceLabel() << " (" << TSystem::vrSensorLabel[sensorID] << "):\n"
		       << TSystem::SUM_ENERGY_LABEL << "," << std::fixed << std::setprecision (6) << energyConsumption << '\n'
		       << "MAX power [W]" << "," << maxWatts[sensorID] << '\n'
		       << "MIN power [W]" << "," << minWatts[sensorID] << '\n'
		       << "AVG power [W]" << "," << avgWatts << '\n'
		       << "ID of first sample" << "," << firstVR << '\n'
		       << "ID of last sample" << "," << lastVR-1 << std::endl;
		}
	}
	
	if(printSamples && staticRegion)
		samplesStream.close();

	delete [] energy;
	delete [] minWatts;
	delete [] maxWatts;
	
	clock_gettime(CLOCK_REALTIME, &time2);
	MERIC_INFO << "MERIC record saved in: " << mutils::timeToDouble(time2 - time1) <<" s\n";
}

template class meric::SamplesStore <HDEEM>;
template class meric::SamplesStore <JETSON>;
template class meric::SamplesStore <THUNDER>;
template class meric::SamplesStore <DAVIDE>;
template class meric::SamplesStore <HWMON>;
template class meric::SamplesStore <RAPL>;