
#include "timestore.h"
#include <cmath>

using namespace meric;

void TimeStore::addStart(int region)	//OK
{
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	_records.push_back(TimeRecord(region, time));
}

void TimeStore::addEnd(int region)	//OK
{
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	_records.push_back(TimeRecord(region, time));
}

void TimeStore::computePeriods()	//OK
{
	if (wrapperType == JETSON)													//TODO odtsranit z ** jinych trid ** dotaz na frekvenci a time adjustment
	{	_samples_freq = JETSON::FREQUENCY;
		JETSON::setTimeAdjustment(_start);
		_timeAdjustment = 0;
	}	
	else if (wrapperType == HDEEM)
	{
		_samples_freq = HDEEM::FREQUENCY;
		_timeAdjustment = HDEEM::setTimeAdjustment(_start);
	}

}

void TimeStore::fillRegionStats(RegionStats &stats, size_t iStarts, size_t iEnd)	//co s tim errorem ?
{
	size_t firstBlade, lastBlade;
	size_t firstVR, lastVR;
	double * energy;
	double * min;
	double * max;
	std::string msg = "";
	stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	if (wrapperType == HDEEM)
	{
		energy = new double [HDEEM::PROBE_NUMBER];
		min = new double [HDEEM::PROBE_NUMBER];
		max = new double [HDEEM::PROBE_NUMBER];
		msg = HDEEM::getData(mutils::timeToDouble(_records[iStarts].time - _start), stats.duration, &firstBlade, &lastBlade, &firstVR, &lastVR, energy, min, max);
	}
	else if (wrapperType == JETSON)
	{
		energy = new double [JETSON::PROBE_NUMBER];
		min = new double [JETSON::PROBE_NUMBER];
		max = new double [JETSON::PROBE_NUMBER];
		msg = JETSON::getData(mutils::timeToDouble(_records[iStarts].time), stats.duration, &firstBlade, &lastBlade, &firstVR, &lastVR, energy, min, max);
	}

	if (msg != "")
		std::cerr << msg << _name[_records[iStarts].region] << std::endl;
/*
	if (lastBlade <= firstBlade)
	{
		std::cerr << "MERIC ERROR: No samples, probably too short ("<<stats.duration<<"s) region "<< _name[_records[iStarts].region]<<"\n";
	}
*/
	stats.energy = 0;
	if(lastBlade > 0)
		stats.energy = stats.duration * energy[0] / (lastBlade - firstBlade);

	delete [] energy;
	delete [] min;
	delete [] max;
}

void TimeStore::fillRegionStats(DetailedRegionStats &stats, size_t iStarts, size_t iEnd)
{
	size_t probeNum;
	if (wrapperType == HDEEM)
		probeNum = HDEEM::PROBE_NUMBER;
	else if (wrapperType == JETSON)
		probeNum = JETSON::PROBE_NUMBER;										//potrebuji probeNum ?

//	std::vector<double> energy(probeNum, 0);
	
	std::string msg = "";
	size_t firstBlade, lastBlade;
	size_t firstVR, lastVR;
	double * energy;
	double * min;
	double * max;
	stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	if (wrapperType == HDEEM)
	{
		energy = new double [HDEEM::PROBE_NUMBER];
		min = new double [HDEEM::PROBE_NUMBER];
		max = new double [HDEEM::PROBE_NUMBER];
		msg = HDEEM::getData(mutils::timeToDouble(_records[iStarts].time - _start), stats.duration, &firstBlade, &lastBlade, &firstVR, &lastVR, energy, min, max);
	}
	else if (wrapperType == JETSON)
	{
		energy = new double [JETSON::PROBE_NUMBER];
		min = new double [JETSON::PROBE_NUMBER];
		max = new double [JETSON::PROBE_NUMBER];
		msg = JETSON::getData(mutils::timeToDouble(_records[iStarts].time), stats.duration, &firstBlade, &lastBlade, &firstVR, &lastVR, energy, min, max);
	}

	if (msg != "")
		std::cerr << msg <<  _name[_records[iStarts].region]<< std::endl;
	
// err check																	//TODO
	if (lastBlade <= firstBlade)
	{
		std::cerr << "MERIC ERROR: No HDEEM samples, probably too short ("<<stats.duration<<"s) region "<< _name[_records[iStarts].region]<<"\n";
	}
	else if (lastVR <= firstVR)
	{
		std::cerr << "MERIC WARNING: No HDEEM samples, probably too short ("<<stats.duration<<"s) detailed region "<< _name[_records[iStarts].region]<<"\n";
	}


	for(size_t i=0; i < HDEEM::PROBE_NUMBER; i++)								//TODO probeNum - stats je automaticky nastaveno na sirku hdeem
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
		for (size_t it = 1; it < probeNum; it++)
		{
			stats.energy[it] = stats.duration * energy[it] / (lastVR - firstVR);
		}
	}
	
	delete [] energy;
	delete [] min;
	delete [] max;
}

void TimeStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	timespec time1, time2;
	clock_gettime(CLOCK_REALTIME, &time1);
		
    ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	if (wrapperType == HDEEM)
		ss << "# Job info - " << HDEEM::storeLabel << std::endl;
	else if (wrapperType == JETSON)
		ss << "# Job info - " << JETSON::storeLabel << std::endl;
	ss << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << _records[iEnd].time - _records[iStarts.back()].time << std::endl;

	size_t firstBlade, lastBlade;
	size_t firstVR, lastVR;
	double * energy;
	double * minWatts;
	double * maxWatts;
    double avgWatts = 0.0;
    double energyConsumption = 0.0;

	std::string msg = "";
	double duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts.back()].time);
	if (wrapperType == HDEEM)
	{
		energy = new double [HDEEM::PROBE_NUMBER];
		minWatts = new double [HDEEM::PROBE_NUMBER];
		maxWatts = new double [HDEEM::PROBE_NUMBER];
		msg = HDEEM::getData(mutils::timeToDouble(_records[iStarts.back()].time - _start), duration, &firstBlade, &lastBlade, &firstVR, &lastVR, energy, minWatts, maxWatts);
	}
	else if (wrapperType == JETSON)
	{
		energy = new double [JETSON::PROBE_NUMBER];
		minWatts = new double [JETSON::PROBE_NUMBER];
		maxWatts = new double [JETSON::PROBE_NUMBER];
		msg = JETSON::getData(mutils::timeToDouble(_records[iStarts.back()].time), duration, &firstBlade, &lastBlade, &firstVR, &lastVR, energy, minWatts, maxWatts); 	//jine volani pro oba pripady - reseni _start na 0.0 ??
	}
	
	if (msg != "")
		std::cerr << msg <<  _name[_records[iStarts.back()].region]<< std::endl;

	MERIC_INFO << _name[_records[iStarts.back()].region] << " FIRST: "<< firstBlade << " in time "<< int(mutils::timeToDouble(_records[iStarts.back()].time)) << std::endl;
	MERIC_INFO << _name[_records[iStarts.back()].region] << " LAST:  "<< lastBlade << " in time "<< int(mutils::timeToDouble(_records[iEnd].time))  << std::endl;

    //////////////////
    // Blade values //
    //////////////////

	ss << "# Store CONTINUAL 1" << std::endl;
	
	if(lastBlade > 0 && lastBlade > firstBlade)
	{
		if (printSamples)
		{
			ss << "# Blade samples" << std::endl;
			std::vector<double> samples;
			if (wrapperType == HDEEM)
				HDEEM::getSamplesList(0, firstBlade, lastBlade, samples);
			else if (wrapperType == JETSON)
				JETSON::getSamplesList(0, firstBlade, lastBlade, samples);
			for (size_t it = firstBlade; it < lastBlade; it++)
				ss << it << "," << samples[it] << std::endl;
		}
		avgWatts = energy[0]/(lastBlade-firstBlade);
		energyConsumption = duration * avgWatts;
	} else {
		firstBlade = 0;
		lastBlade = 1;
		std::cerr << "MERIC ERROR: No samples, probably too short ("<<duration<<"s) region "<< _name[_records[iStarts.back()].region]<<"\n";
	}
	
	ss << "# Blade summary" << std::endl
	   << "Energy consumption [J] (Samples)" << "," << energyConsumption << std::endl
       << "Maximal power [W] (Samples)" << "," << maxWatts[0] << std::endl
       << "Minimal power [W] (Samples)" << "," << minWatts[0] << std::endl
       << "Average power [W] (Samples)" << "," << avgWatts << std::endl
       << "ID of first blade sample" << "," << firstBlade << std::endl
       << "ID of last blade sample" << "," << lastBlade-1 << std::endl;

	delete [] energy;
	delete [] minWatts;
	delete [] maxWatts;
	
	clock_gettime(CLOCK_REALTIME, &time2);
	MERIC_INFO << "MERIC record saved in: " << mutils::timeToDouble(time2 - time1) <<" s\n";
}
