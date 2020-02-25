

#include "hdeemwrapper.h"

using namespace meric;

std::vector<std::string> HDEEM::vrSensorLabel;

#ifdef HAVE_HDEEM_H

hdeem_bmc_data_t HDEEM::bmc;
hdeem_global_reading_t HDEEM::_samples;
size_t HDEEM::_timeAdjustment = 0;
double HDEEM::_startTime;

void HDEEM::setTimeAdjustment(timespec start)
{
	hdeem_status_t status;
	HDEEM::getStatus(status);
	_timeAdjustment = FREQUENCY * mutils::timeToDouble(start - status.start_time_blade) -1;
	_startTime = mutils::timeToDouble(start);
	MERIC_INFO << "SAMPLES TIME ADJUSTMENT: " << _timeAdjustment << std::endl;
}

std::string HDEEM::getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double energy[], double minWatts[], double maxWatts[])
{
	std::string errMessage = "";
	*firstBlade = std::ceil((startTime-_startTime) * FREQUENCY) + _timeAdjustment;
	*lastBlade  = std::floor(duration * FREQUENCY) + *firstBlade;

	MERIC_INFO << "FIRST " << *firstBlade << std::endl;
	MERIC_INFO << "LAST " << *lastBlade << std::endl;

	if (*lastBlade > _samples.nb_blade_values)	//TODO
	{
		errMessage = "HDEEM ERROR - missing samples\n";
		*lastBlade = -1;
	}

	*firstVR = *firstBlade/(FREQUENCY/FREQUENCY_VR) + VRDELAY;
	*lastVR  = *lastBlade/(FREQUENCY/FREQUENCY_VR)  + VRDELAY;
	if (*lastVR > _samples.nb_vr_values)
		*lastVR = _samples.nb_vr_values;

	for (unsigned int sensorID = 0; sensorID < PROBE_NUMBER; sensorID++)
	{
		energy[sensorID]   = 0.0;
		maxWatts[sensorID] = 0.0;
		minWatts[sensorID] = 0.0;
	}

	auto getEnergySamples = [&] (size_t firstSample, size_t lastSample, size_t sensorID)
	{
		if (sensorID == 0)
			maxWatts[sensorID] = _samples.blade_power[firstSample].value[sensorID];
		else
			maxWatts[sensorID] = _samples.vr_power[firstSample].value[sensorID-1];
		minWatts[sensorID] = maxWatts[sensorID];
		double valTmp;
		for (size_t it = firstSample; it < lastSample; it++)
		{
			if (sensorID == 0)
				valTmp = _samples.blade_power[it].value[sensorID];
			else
				valTmp = _samples.vr_power[it].value[sensorID-1];
			if (valTmp > maxWatts[sensorID])
				maxWatts[sensorID] = valTmp;
			else if (valTmp < minWatts[sensorID])
				minWatts[sensorID] = valTmp;
			energy[sensorID] += valTmp;
		}
	};

	//VR
	if(*lastVR > 0 && *lastVR > *firstVR)
	{
		for (unsigned int sensorID = 1; sensorID < vrSensorLabel.size(); sensorID++)
			getEnergySamples(*firstVR, *lastVR, sensorID);
	}else
	{
		*firstVR = -1;
		*lastVR = -1;
		errMessage = "MERIC ERROR: No HDEEM samples, probably too short ("+std::to_string(duration)+"s) detailed region";
	}

	//BLADE
	if(*lastBlade > 0 && *lastBlade > *firstBlade)
		getEnergySamples(*firstBlade, *lastBlade, 0);
	else
	{
		*firstBlade = -1;
		*lastBlade = -1;
		errMessage = "MERIC ERROR: No HDEEM samples, probably too short ("+std::to_string(duration)+"s) region";
	}
	
	return errMessage;	
}


void HDEEM::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double>  & samples)
{
	if (sensorId == 0)
	{
		for (size_t i = first; i < last; i++)
			samples.push_back(_samples.blade_power[i].value[0]);
	}
	else
	{
		for (size_t i = first; i < last; i++)
			samples.push_back(_samples.vr_power[i].value[sensorId-1]);
	}
}

double HDEEM::getResultValue(double startValue, double stopValue, double runtime, std::string counter)
{
	double energy = stopValue - startValue;

	if (energy <= 0)
	{
		std::cerr << "MERIC ERROR: No energy consumption, probably too short ("<<runtime<<"s) region "<< counter<<"\n";
	}
	else if (runtime < 0.01)
	{
		std::string info;
		if (runtime > 0.001)	// too short region in all cases
			info = "";
		else
			info = " detailed";
		std::cerr << "MERIC WARNING: very short ("<<runtime<< "s)"<< info <<" region " << counter << " results might be skewed\n";
	}
	return energy;
}

#else

size_t HDEEM::energy = 0;

#endif

std::string HDEEM::getSummaryCounterName()
{
	return "";
}

std::string HDEEM::getSourceLabel()
{
	return "HDEEM";
}




