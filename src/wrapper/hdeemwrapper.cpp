

#include "hdeemwrapper.h"

using namespace meric;

#ifndef USE_DUMMY

hdeem_bmc_data_t HDEEM::bmc;
hdeem_global_reading_t HDEEM::_samples;
size_t HDEEM::_timeAdjustment = 0;

size_t HDEEM::setTimeAdjustment(timespec start)
{
	hdeem_status_t status;
	HDEEM::getStatus(status);
	_timeAdjustment = FREQUENCY * mutils::timeToDouble(start - status.start_time_blade) -1;
	MERIC_INFO << "SAMPLES TIME ADJUSTMENT: " << _timeAdjustment << std::endl;
	return _timeAdjustment;
}

std::string HDEEM::getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double energy[], double minWatts[], double maxWatts[])
{

	std::string errMessage = "";
	*firstBlade = std::ceil(startTime * FREQUENCY) + _timeAdjustment;
	*lastBlade  = std::floor(duration * FREQUENCY) + *firstBlade;

//MERIC_INFO << "FIRST " << *firstBlade << std::endl;
//MERIC_INFO << "LAST " << *lastBlade << std::endl;

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
		maxWatts[sensorID] = _samples.blade_power[firstSample].value[sensorID];
		minWatts[sensorID] = maxWatts[sensorID];
		double valTmp;
		for (size_t it = firstSample; it < lastSample; it++)
		{
			if (sensorID == 0)
				valTmp = _samples.blade_power[it].value[sensorID];
			else
				valTmp = _samples.vr_power[it].value[sensorID];
			if (valTmp > maxWatts[sensorID])
				maxWatts[sensorID] = valTmp;
			else if (valTmp < minWatts[sensorID])
				minWatts[sensorID] = valTmp;
			energy[sensorID] += valTmp;
		}
	};

	if(*lastBlade > 0 && *lastBlade > *firstBlade)
	{
		getEnergySamples(*firstBlade, *lastBlade, 0);
		for (unsigned int sensorID = 1; sensorID < PROBE_NUMBER; sensorID++)
			getEnergySamples(*firstVR, *lastVR, sensorID);
	}else
	{
		*firstBlade = -1;
		*lastBlade = -1;
		errMessage = "MERIC ERROR: No HDEEM samples, probably too short ("+std::to_string(duration)+"s) region";
	}

	return errMessage;	
}


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





