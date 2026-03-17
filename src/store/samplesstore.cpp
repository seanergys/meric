
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
	BaseWrapper::SamplerRecord_t samplerRecord;

	std::string msg = "";
	const double duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	msg = TSystem::getData(mutils::timeToDouble(_records[iStarts].time), duration, samplerRecord);

	if (msg != "")
		std::cerr << msg <<  _name[_records[iStarts].region] << std::endl;

	stats.totalEnergy = 0.0;

	for(const auto &sampler: samplerRecord)
	{
		if(sampler.last <= 0 || sampler.last <= sampler.first)
		{
			stats.energy.push_back(0.0);
			continue;
		}

		const double energy = duration * sampler.energy / (sampler.last - sampler.first);
		stats.energy.push_back(energy);
		stats.totalEnergy += sampler.info.IsAccumulative() ? energy : 0.0;
	}
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

	double avgWatts = 0.0;
	double energyConsumption = 0.0;
	BaseWrapper::SamplerRecord_t samplerRecord;

	std::string msg = "";
	double duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts.back()].time);
	msg = TSystem::getData(mutils::timeToDouble(_records[iStarts.back()].time), duration, samplerRecord);
	
	if (msg != "" && msg.find("detailed") == std::string::npos)
		std::cerr << msg <<  _name[_records[iStarts.back()].region]<< std::endl;

	// MERIC_INFO << _name[_records[iStarts.back()].region] << " FIRST: " << firstBlade << " in time " << int(mutils::timeToDouble(_records[iStarts.back()].time)) << std::endl;
	// MERIC_INFO << _name[_records[iStarts.back()].region] << " LAST:  " << lastBlade << " in time " << int(mutils::timeToDouble(_records[iEnd].time))  << std::endl;
	
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

	int samplerErrorCount = 0;

	for(const auto &sampler: samplerRecord)
	{
		if(sampler.last == 0 || sampler.last <= sampler.first)
		{
			if(samplerErrorCount == 0)
			{
				ss << "# SAMPLES - " << TSystem::getSourceLabel() << " (" << sampler.name << ") - ERROR\n";
			}

			samplerErrorCount++;
			continue;
		}

		if(printSamples && staticRegion)
		{
			samplesStream << "# SAMPLES - " << TSystem::getSourceLabel() << " (" << sampler.name << ") power [W]:\n";
			std::vector<double> samples;
			TSystem::getSamplesList(sampler.id, sampler.first, sampler.last, samples);

			for(size_t i = 0; i < samples.size(); ++i)
				samplesStream << (sampler.first + i) << "," << samples[i] << "\n";
		}

		avgWatts = sampler.energy / (sampler.last - sampler.first);
		energyConsumption = duration * avgWatts;

		ss << "# SAMPLES - " << TSystem::getSourceLabel() << " (" << sampler.name << "):\n"
			<< TSystem::SUM_ENERGY_LABEL << "," << std::fixed << std::setprecision (6) << energyConsumption << '\n'
		    << "MAX power [W]" << "," << sampler.max << '\n'
		    << "MIN power [W]" << "," << sampler.min << '\n'
		    << "AVG power [W]" << "," << avgWatts << '\n'
		    << "ID of first sample" << "," << sampler.first << '\n'
		    << "ID of last sample" << "," << (sampler.last - 1) << std::endl;
	}
	
	if(printSamples && staticRegion)
		samplesStream.close();
	
	clock_gettime(CLOCK_REALTIME, &time2);
	MERIC_INFO << "MERIC record saved in: " << mutils::timeToDouble(time2 - time1) <<" s\n";
}

template class meric::SamplesStore <HDEEM>;
template class meric::SamplesStore <JETSON>;
template class meric::SamplesStore <THUNDER>;
template class meric::SamplesStore <DAVIDE>;
template class meric::SamplesStore <HWMON>;
template class meric::SamplesStore <RAPL>;
template class meric::SamplesStore <NVML>;