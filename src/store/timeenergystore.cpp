
#include "timeenergystore.h"
#include "../basis/sysfsutils.h"
#include <cmath>

using namespace meric;

void TimeEnergyStore::addStart(int region, RegionParameters configuration)
{
	struct timespec time;
	std::array<double, HDEEM::PROBE_NUMBER> energy;

	HDEEM::getDetailedCounterValues(energy);
	clock_gettime(CLOCK_REALTIME, &time);

	_records.push_back(TimeEnergyRecord(region, time, energy, configuration));
}

void TimeEnergyStore::addEnd(int region)
{
	struct timespec time;
	std::array<double, HDEEM::PROBE_NUMBER> energy;

	clock_gettime(CLOCK_REALTIME, &time);
    HDEEM::getDetailedCounterValues(energy);

	_records.push_back(TimeEnergyRecord(region, time, energy, _records.back().configuration));
}


void TimeEnergyStore::fillRegionStats(DebugRegionStats &stats, size_t iStarts, size_t iEnd)
{
	BaseWrapper::SamplerRecord_t samplerRecord;

	std::string msg = "";
	stats.mericDuration = mutils::timeToDouble(_records[iEnd].clockTime - _records[iStarts].clockTime);
	msg = HDEEM::getData(mutils::timeToDouble(_records[iStarts].clockTime - _start), stats.mericDuration, samplerRecord);

	for (size_t i = 0; i < samplerRecord.size(); ++i)
	{
		const auto &sampler = samplerRecord[i];

		stats.energyStatsStructure[i] = (_records[iEnd].energy[sampler.id] - _records[iStarts].energy[sampler.id]);
		if (sampler.last <= 0 || sampler.last <= sampler.first)
		{
			stats.energy[i] = 0.0;
			continue;
		}

		stats.energy[i] = (stats.mericDuration * sampler.energy / (sampler.last - sampler.first));
	}
}

void TimeEnergyStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++)
	{
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	double avgWatts = 0.0;
	double energyConsumption = 0.0;
	BaseWrapper::SamplerRecord_t samplerRecord;

	std::string msg = "";
	double duration = mutils::timeToDouble(_records[iEnd].clockTime - _records[iStarts.back()].clockTime);
	msg = HDEEM::getData(mutils::timeToDouble(_records[iStarts.back()].clockTime), duration, samplerRecord);

	if (msg != "")
		std::cerr << msg <<  _name[_records[iStarts.back()].region]<< std::endl;

	bool staticRegion = false;
	std::ofstream samplesStream;
	if (printSamples)
	{
		const size_t staticRegionSufixLength = std::string("_static").length();
		staticRegion = _name[_records[iStarts.back()].region].length() > staticRegionSufixLength && _name[_records[iStarts.back()].region].compare(_name[_records[iStarts.back()].region].length() - staticRegionSufixLength, staticRegionSufixLength, "_static") == 0;
		if (staticRegion)
		{
			const std::string samplesDir = outputDirectory+"/"+_name[_records[iStarts.back()].region]+"/samples/";
			sysfsutils::CreateDir(samplesDir);
			samplesStream.open(samplesDir + (outputPrefix.size() ? outputPrefix + "_" : "") + std::to_string(_initCounter) + ".csv", std::ofstream::app);
		}
	}

	for (const auto &sampler : samplerRecord)
	{
		if (printSamples && staticRegion)
		{
			samplesStream << "# SAMPLES - " << HDEEM::getSourceLabel() << " (" << sampler.name << ") power [W]:\n";
			std::vector<double> samples;
			HDEEM::getSamplesList(sampler.id, sampler.first, sampler.last, samples);
			for (size_t i = 0; i < samples.size(); ++i)
				samplesStream << (sampler.first + i) << "," << samples[i] << std::endl;
		}
		avgWatts = sampler.energy / (sampler.last - sampler.first);
		energyConsumption = duration * avgWatts;

		if (_records[iEnd].energy[sampler.id + 1] - _records[iStarts.back()].energy[sampler.id + 1] <= 0)
		{
			std::cerr << "MERIC WARNING: No energy consumption, probably too short (" << _records[iEnd].clockTime - _records[iStarts.back()].clockTime
					  << "s) detailed region " << _name[_records[iStarts.back()].region] << " - " << sampler.name << "\n";
		}

		ss << "# COUNTERS - " << HDEEM::getSourceLabel() << " (" << sampler.name << "):\n"
		   << HDEEM::SUM_ENERGY_LABEL << " - counters" << "," << _records[iEnd].energy[sampler.id + 1] - _records[iStarts.back()].energy[sampler.id + 1]
		   << std::endl
		   << "# SAMPLES - " << HDEEM::getSourceLabel() << " (" << sampler.name << "):\n"
		   << HDEEM::SUM_ENERGY_LABEL << " - samples" << "," << energyConsumption << std::endl
		   << "MAX power [W]" << "," << sampler.max << std::endl
		   << "MIN power [W]" << "," << sampler.min << std::endl
		   << "AVG power [W]" << "," << avgWatts << std::endl
		   << "ID of first sample" << "," << sampler.first << std::endl
		   << "ID of last sample" << "," << (sampler.last - 1) << std::endl;
	}
}




