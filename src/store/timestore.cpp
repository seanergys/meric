
#include "timestore.h"
#include <cmath>

using namespace meric;

template <class TSystem> void TimeStore<TSystem>::addStart(int region)
{
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	_records.push_back(TimeRecord(region, time));
}

template <class TSystem> void TimeStore<TSystem>::addEnd(int region)
{
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	_records.push_back(TimeRecord(region, time));
}

template <class TSystem> void TimeStore<TSystem>::computePeriods()
{
	TSystem::setTimeAdjustment(_start);
}

template <class TSystem> void TimeStore<TSystem>::fillRegionStats(RegionStats &stats, size_t iStarts, size_t iEnd)
{
	size_t firstBlade, lastBlade;
	size_t firstVR, lastVR;
	double * energy;
	double * min;
	double * max;
	std::string msg = "";
	stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);

	energy = new double [TSystem::PROBE_NUMBER];
	min = new double [TSystem::PROBE_NUMBER];
	max = new double [TSystem::PROBE_NUMBER];
	msg = TSystem::getData(mutils::timeToDouble(_records[iStarts].time), stats.duration, &firstBlade, &lastBlade, &firstVR, &lastVR, energy, min, max);

	if (msg != "")
		std::cerr << msg << _name[_records[iStarts].region] << std::endl;

	stats.energy = 0;
	if(lastBlade > 0)
		stats.energy = stats.duration * energy[0] / (lastBlade - firstBlade);

	delete [] energy;
	delete [] min;
	delete [] max;
}

template <class TSystem> void TimeStore<TSystem>::fillRegionStats(DetailedRegionStats &stats, size_t iStarts, size_t iEnd)
{
	std::string msg = "";
	size_t firstBlade, lastBlade;
	size_t firstVR, lastVR;
	double * energy;
	double * min;
	double * max;
	stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	energy = new double [TSystem::PROBE_NUMBER];
	min = new double [TSystem::PROBE_NUMBER];
	max = new double [TSystem::PROBE_NUMBER];
	msg = TSystem::getData(mutils::timeToDouble(_records[iStarts].time), stats.duration, &firstBlade, &lastBlade, &firstVR, &lastVR, energy, min, max);

	if (msg != "")
		std::cerr << msg <<  _name[_records[iStarts].region]<< std::endl;
	

	for(size_t i=0; i < HDEEM::PROBE_NUMBER; i++) //stats.energy has always HDEEM::PROBE_NUMBER size
	{
		stats.energy[i] = 0;
	}
	
	//DC
	if(lastBlade > 0 && lastBlade > firstBlade)
	{
		stats.energy[0] = stats.duration * energy[0] / (lastBlade - firstBlade);
	}
	
	//VR
	if(lastVR > 0 && lastVR > firstVR)
	{
		for (size_t it = 1; it < TSystem::PROBE_NUMBER; it++)
		{
			stats.energy[it] = stats.duration * energy[it] / (lastVR - firstVR);
		}
	}
	
	delete [] energy;
	delete [] min;
	delete [] max;
}

template <class TSystem> void TimeStore<TSystem>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	timespec time1, time2;
	clock_gettime(CLOCK_REALTIME, &time1);
		
    ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

//	ss << "# Job info - " << TSystem::storeLabel << std::endl;
//	ss << "JOB ID," << _jobID << "." << _stepID  << std::endl
//	   << "Nodelist," << _nodelist << std::endl
//	   << "Runtime of function [s]," << _records[iEnd].time - _records[iStarts.back()].time << std::endl;

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

    //////////////////
    // Blade values //
    //////////////////

	ss << "# Store CONTINUAL 1 - " << TSystem::storeLabel << std::endl;
	
	if(lastBlade > 0 && lastBlade > firstBlade)
	{
		if (printSamples)
		{
			ss << "# Blade samples" << std::endl;
			std::vector<double> samples;
			TSystem::getSamplesList(0, firstBlade, lastBlade, samples);
			size_t it = firstBlade;
			for (size_t i = 0; i < samples.size(); i++)
				ss << it++ << "," << samples[i] << std::endl;
		}
		avgWatts = energy[0]/(lastBlade-firstBlade);
		energyConsumption = duration * avgWatts;
	} else {
		firstBlade = 0;
		lastBlade = 1;
		std::cerr << "MERIC ERROR: No samples, probably too short ("<<duration<<"s) region "<< _name[_records[iStarts.back()].region]<<"\n";
	}
	
	ss << "# Blade summary" << std::endl
	   << "Runtime of function [s]," << duration << std::endl
	   << "Energy consumption [J]" << "," << energyConsumption << std::endl
       << "Maximal power [W]" << "," << maxWatts[0] << std::endl
       << "Minimal power [W]" << "," << minWatts[0] << std::endl
       << "Average power [W]" << "," << avgWatts << std::endl
       << "ID of first blade sample" << "," << firstBlade << std::endl
       << "ID of last blade sample" << "," << lastBlade-1 << std::endl;

	delete [] energy;
	delete [] minWatts;
	delete [] maxWatts;
	
	clock_gettime(CLOCK_REALTIME, &time2);
	MERIC_INFO << "MERIC record saved in: " << mutils::timeToDouble(time2 - time1) <<" s\n";
}

template class TimeStore <HDEEM>;
template class TimeStore <JETSON>;
template class TimeStore <THUNDER>;
