

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
std::string DAVIDE::url_samples;
RestClient::Connection* DAVIDE::conn_samples;

void DAVIDE::init()
{

	char hostname[50];
   	gethostname(hostname, 50);
	node_n = std::string(hostname); //"davide43"; 
	MERIC_INFO << "Hostname : " << node_n << "\n";

	metric = "power";
	url_samples = std::string("http://") + std::string(KAIROSDB_SERVER) + std::string(":") + std::string(KAIROSDB_PORT) + std::string("/api/v1/") + std::string("datapoints/query");
	// initialize RestClient
	RestClient::init();

	MERIC_INFO << "Server : " << KAIROSDB_SERVER << "\n"; 
	MERIC_INFO << "URL is : " << url_samples << "\n";
	// get a connection object
	conn_samples = new RestClient::Connection(url_samples);

	// configure basic auth
	conn_samples->SetBasicAuth("admin", "k41r0sdb768");

	// set connection timeout to 5s
	conn_samples->SetTimeout(30);
}


//metoda pro ziskani energie pro pozadovany region na zaklade pocatecniho a koncoveho casu
std::string DAVIDE::getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double energy[], double minWatts[], double maxWatts[])
{
	std::string errMessage = "";


	long long t0 = time_samples[0]; 
	long long ts = (long long)(FREQUENCY*startTime);
	long long dur_t = (long long)(FREQUENCY*duration);
	*firstBlade = (size_t)(ts - t0); 
	*lastBlade = (size_t)((ts - t0) + dur_t);  
	size_t n_samples = *lastBlade - *firstBlade;

	MERIC_INFO << "StartTime  : " << t0 << "\n";
	MERIC_INFO << "Duration   : " << dur_t << "\n";
	MERIC_INFO << "firstBlade : " << *firstBlade << "\n";
	MERIC_INFO << "lastBlade  : " << *lastBlade  << "\n";
	MERIC_INFO << "Samples    : " << n_samples << "\n";

	// *firstBlade = std::ceil(startTime * FREQUENCY) + _timeAdjustment;
	// *lastBlade  = std::floor(duration * FREQUENCY) + *firstBlade;

	MERIC_INFO << "FIRST " << *firstBlade << std::endl;
	MERIC_INFO << "LAST " << *lastBlade << std::endl;

	energy[0]   = 0.0;
	maxWatts[0] = power_samples[*firstBlade];
	minWatts[0] = power_samples[*firstBlade];

	double valTmp = 0.0;
	for (size_t it=*firstBlade; it <*lastBlade; it++)
	{
		valTmp = power_samples[it];

		if (valTmp > maxWatts[0])
			maxWatts[0] = valTmp;
		else if (valTmp < minWatts[0])
			minWatts[0] = valTmp;
		energy[0] += valTmp;
	}
	energy[0] = energy[0] / ((double)(n_samples)) * duration;

	MERIC_INFO << "minWatts : " << minWatts[0] << "\n";
	MERIC_INFO << "maxWatts : " << maxWatts[0] << "\n";
	MERIC_INFO << "avgWatts : " << energy[0] / ((double)(n_samples)) << "\n";	
	MERIC_INFO << "energy   : " << energy[0] << "\n";

	return errMessage;	
}

//sensorId bude 0 - blade
void DAVIDE::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double>  & samples)
{

	for (size_t i = first; i < last; i++)
		samples.push_back(power_samples[i]);
}

//#endif







