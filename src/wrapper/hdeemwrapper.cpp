

#include "hdeemwrapper.h"

using namespace meric;

std::vector<std::string> HDEEM::vrSensorLabel;
bool HDEEM::detailedMode = false;
const std::string HDEEM::SUM_ENERGY_LABEL = "Energy consumption [J]";

bool HDEEM::isAvailable()
{
#ifdef HAVE_HDEEM_H
	return true;
#else
	return false;
#endif // HAVE_HDEEM_H
}

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

std::string HDEEM::getData(double startTime, double duration, SamplerRecord_t &record)
{
	std::string errMessage = "";
	size_t firstBlade = std::ceil((startTime-_startTime) * FREQUENCY) + _timeAdjustment;
	size_t lastBlade  = std::floor(duration * FREQUENCY) + firstBlade;

	MERIC_INFO << "FIRST " << firstBlade << std::endl;
	MERIC_INFO << "LAST " << lastBlade << std::endl;

	if (lastBlade > _samples.nb_blade_values)	//TODO
	{
		errMessage = "HDEEM ERROR - missing samples\n";
		lastBlade = -1;
	}

	for (unsigned int sensorID = 0; sensorID < PROBE_NUMBER; sensorID++)
	{
		record.push_back(SamplerData_t());
		record.back().id   = sensorID;
		record.back().name = vrSensorLabel[sensorID];

		record.back().energy = 0.0;
		record.back().min = 0.0;
		record.back().max = 0.0;
		record.back().first = 0;
		record.back().last  = 0;

		record.back().info.type   = SourceInfo_t::TYPE_POWER;

		if(sensorID == 0)
		{
			record.back().info.flags |= SourceInfo_t::FLAG_ACCUMULATIVE_BIT;
		}
	}

	auto getEnergySamples = [&] (SamplerData_t &sensor, size_t firstSample, size_t lastSample)
	{
		sensor.first = firstSample;
		sensor.last  = lastSample;

		if (sensor.id == 0)
		{
			sensor.max = _samples.blade_power[firstSample].value[sensor.id];
		}
		else
		{
			sensor.max = _samples.vr_power[firstSample].value[sensor.id - 1];
		}

		sensor.min = sensor.max;

		double valTmp;
		for (size_t it = firstSample; it < lastSample; it++)
		{
			if (sensor.id == 0)
			{
				valTmp = _samples.blade_power[it].value[sensor.id];
			}
			else
			{
				valTmp = _samples.vr_power[it].value[sensor.id - 1];
			}

			sensor.max = std::max(sensor.max, valTmp);
			sensor.min = std::min(sensor.min, valTmp);

			sensor.energy += valTmp;
		}
	};

	//Voltage Regulators
	if (detailedMode)
	{
		size_t firstVR = firstBlade / (FREQUENCY / FREQUENCY_VR) + VRDELAY;
		size_t lastVR  = lastBlade / (FREQUENCY / FREQUENCY_VR)  + VRDELAY;
		if (lastVR > _samples.nb_vr_values)
		{
			lastVR = _samples.nb_vr_values;
		}
		
		if(lastVR > 0 && lastVR > firstVR)
		{
			for (unsigned int sensorID = 1; sensorID < vrSensorLabel.size(); sensorID++)
			{
				getEnergySamples(record[sensorID], firstVR, lastVR);
			}
		}
		else
		{
			/*
			*firstVR = -1;
			*lastVR = -1;
			*/
			errMessage = "MERIC ERROR: No HDEEM samples, probably too short ("+std::to_string(duration)+"s) region ";
		}
	}
	else
	{
		/*
		*firstVR = 0;
		*lastVR = 0;
		*/
	}

	//BLADE - error message about missing blade samples is more important than missing VR samples
	if(lastBlade > 0 && lastBlade > firstBlade)
	{
		getEnergySamples(record[0], firstBlade, lastBlade);
	}
	else
	{
		/*
		*firstBlade = -1;
		*lastBlade = -1;
		*/
		errMessage = "MERIC ERROR: No HDEEM samples, probably too short ("+std::to_string(duration)+"s) region ";
	}
	
	return errMessage;	
}


void HDEEM::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double>  & samples)
{
	if (sensorId == 0)
	{
		for (size_t i = first; i < last; i++)
		{
			samples.push_back(_samples.blade_power[i].value[0]);
		}
	}
	else
	{
		for (size_t i = first; i < last; i++)
		{
			samples.push_back(_samples.vr_power[i].value[sensorId-1]);
		}
	}
}

double HDEEM::getResultValue(double startValue, double stopValue, double runtime, int & cumulative, std::string counter, std::string regionName)
{
	cumulative = 0;
	double energy = stopValue - startValue;

	if (energy <= 0)
	{
		std::cerr << "MERIC ERROR: No energy consumption, probably too short ("<<runtime<<"s) region '"<< regionName <<"'\n";
		energy = .0;
	}
	else if (runtime < 0.01)
	{
		std::string info = "";
		if (detailedMode && runtime < 0.001)
		{
			info = " detailed";
		}
		std::cerr << "MERIC WARNING: very short ("<<runtime<< "s)"<< info <<" region '" << regionName << "' results might be skewed\n";
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





