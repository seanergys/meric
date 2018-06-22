

#include "davidewrapper.h"

using namespace meric;

//#ifdef HAVE_DAVIDE_H

std::string DAVIDE::node_n;
std::string DAVIDE::metric;
unsigned long long DAVIDE::_startTime;
unsigned long long DAVIDE::_stopTime;
std::vector<long long> DAVIDE::time_samples; // vector will contain all time stamps from the specified interval 
std::vector<double>    DAVIDE::power_samples; // vector will contain power samples corelated to timestamps 
std::string DAVIDE::storeLabel = "davide";
std::vector<std::string> DAVIDE::vrSensorLabel = {"blade"};
//const int DAVIDE::PROBE_NUMBER = vrSensorLabel.size() + 1; 


//metoda pro ziskani energie pro pozadovany region na zaklade pocatecniho a koncoveho casu
std::string DAVIDE::getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double energy[], double minWatts[], double maxWatts[])
{
	std::string errMessage = "";

	*firstBlade = //std::ceil(startTime * FREQUENCY) + _timeAdjustment;
	*lastBlade  = //std::floor(duration * FREQUENCY) + *firstBlade;

	MERIC_INFO << "FIRST " << *firstBlade << std::endl;
	MERIC_INFO << "LAST " << *lastBlade << std::endl;

	energy[0]   = 0.0;
	maxWatts[0] = 0.0;
	minWatts[0] = 0.0;

	for (size_t it=*firstBlade; it <*lastBlade; it++)
	{
		valTmp = power_samples[it];

		if (valTmp > maxWatts)
			maxWatts[0] = valTmp;
		else if (valTmp < minWatts)
			minWatts[0] = valTmp;
		energy[0] += valTmp;
	}

	return errMessage;	
}

//sensorId bude 0 - blade
void DAVIDE::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double>  & samples)
{

	for (size_t i = first; i < last; i++)
		samples.push_back(power_samples[i]);
}

//#endif







