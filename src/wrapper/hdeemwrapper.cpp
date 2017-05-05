

#include "hdeemwrapper.h"

using namespace meric;

#ifndef USE_DUMMY

hdeem_bmc_data_t HDEEM::bmc;
hdeem_global_reading_t HDEEM::_samples;
siize_t HDEEM::_timeAdjustment = 0;

size_t HDEEM::setTimeAdjustment(timespec start)
{
	hdeem_status_t status;
	HDEEM::getStatus(status);
	_timeAdjustment = FREQUENCY * mutils::timeToDouble(start - status.start_time_blade) -1;
	MERIC_INFO << "SAMPLES TIME ADJUSTMENT: " << _timeAdjustment << std::endl;
	return _timeAdjustment;
}

std::string HDEEM::getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double *energy, double *minWatts, double *maxWatts)
{
	std::string errMessage = "";
	*firstBlade = std::ceil(startTime * FREQUENCY) + _timeAdjustment;
	*lastBlade  = std::floor(duration * FREQUENCY) + *firstBlade;

	if (*lastBlade > _samples.nb_blade_values)	//TODO
	{
		errMessage << "HDEEM ERROR - missing samples\n";
		*lastBlade = -1;
	}
	
	*energy = 0.0;
	
	if(*lastBlade > 0 && *lastBlade > *firstBlade)
	{
		*maxWatts = _samples.blade_power[*firstBlade].value[0];
		*minWatts = *maxWatts;
		for (size_t it = *firstBlade; it < *lastBlade; it++)
		{
			double valTmp = _samples.blade_power[it].value[0];
			if (valTmp > *maxWatts)
				*maxWatts = valTmp;
			else if (valTmp < *minWatts)
				*minWatts = valTmp;
			*energy += valTmp;
		}
	}else
	{
		*firstBlade = -1;
		*lastBlade = -1;
		errMessage = "MERIC ERROR: No HDEEM samples, probably too short ("<<duration<<"s) region";
	}

	
	//TODO
	*firstVR    = *firstBlade/(FREQUENCY/FREQUENCY_VR) + VRDELAY;
	*lastVR     = *lastBlade/(FREQUENCY/FREQUENCY_VR)  + VRDELAY;
	//...

	//
	return errMessage;	
}

/*
void HDEEM::getDataVR(size_t firstBlade, size_t lastBlade, size_t &firstVR, size_t &lastVR, double &energy, double &minWatts, double &maxWatts)
{

	firstVR    = firstBlade/(FREQUENCY/FREQUENCY_VR) + VRDELAY;
	lastVR     = lastBlade/(FREQUENCY/FREQUENCY_VR)  + VRDELAY;
	if (lastVR > _samples.nb_vr_values)
		lastVR = _samples.nb_vr_values;

	for (unsigned int sensorID = 0; sensorID < PROBE_NUMBER-1; sensorID++)
	{
		energy[sensorID]   = 0.0;
		maxWatts[sensorID] = 0.0;
		minWatts[sensorID] = 0.0;

		if(lastVR > 0 && lastVR > firstVR)
		{
			maxWatts[sensorID] = _samples.vr_power[first].value[sensorID];
			minWatts[sensorID] = maxWatts[sensorID];
			for(size_t it = firstVR; it < lastVR; it++)
			{
				double valTmp = _samples.vr_power[it].value[sensorID];
				if (valTmp > maxWatts[sensorID])
					maxWatts[sensorID] = valTmp;
				else if (valTmp < minWatts[sensorID])
					minWatts[sensorID] = valTmp;        
				energy[sensorID] += valTmp;
			}
		}
	}
}
*/
void HDEEM::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> samples)
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


#else

size_t HDEEM::energy = 0;

#endif

	//TODO spojit tyto 3 polozky v jednu
std::string HDEEM::storeLabel = "hdeem";
std::vector<std::string> HDEEM::vrSensorLabel = {"CPU0", "CPU1", "DDR AB", "DDR CD", "DDR EF", "DDR GH"};
//const int HDEEM::PROBE_NUMBER = vrSensorLabel.size() + 1; 





