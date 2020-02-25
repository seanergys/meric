
#ifndef SRC_WRAPPER_DAVIDEWRAPPER_H_
#define SRC_WRAPPER_DAVIDEWRAPPER_H_

#include "../basis/spline.h"
#include "../basis/utils.h"
#include "../basis/json.h"
#include <iostream>
#include <unistd.h>

#ifdef HAVE_RESTCLIENT_CPP_RESTCLIENT_H
	// *** KairosDB setup for DAVIDE monitoring system 
	#define KAIROSDB_SERVER "192.168.1.97"
	#define KAIROSDB_PORT   "8083"
	#define KAIROSDB_USER   "admin"
	#define KAIROSDB_PWD    "k41r0sdb768"

	// *** Rest client headers required to access KairosDD
	#include "restclient-cpp/connection.h"
	#include "restclient-cpp/restclient.h"
#endif

namespace meric {

class DAVIDE {

public:

	static const int FREQUENCY = 1000;
	static const int PROBE_NUMBER = 1;	//no voltage regulators, just blade
	static std::vector<std::string> vrSensorLabel;	//voltage regulator vector
	static std::string storeLabel;

	static std::string getSourceLabel() {	return "DAVIDE";};
	static void deleteSamples() {};	
	static void start(timespec & time)
	{
		clock_gettime(CLOCK_REALTIME, &time);
		_startTime = (unsigned long long)(mutils::timeToDouble(time)*1000.0);
	}
	static void stop(timespec & time)
	{
		clock_gettime(CLOCK_REALTIME, &time);
		_stopTime = (unsigned long long)(mutils::timeToDouble(time)*1000.0);
	}
	static void setTimeAdjustment(timespec start) {};


	#ifdef HAVE_RESTCLIENT_CPP_RESTCLIENT_H
	static void close() { RestClient::disable(); }

	static int getSamples();
	static void init();
	static std::string getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double energy[], double min[], double max[]);
	static void getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> & samples);
	#else
	static void close() {}

	static int getSamples() { return 0; };
	static void init() {};
	static std::string getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double energy[], double min[], double max[]) { return ""; };
	static void getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> & samples) {};
	#endif


private:
	static std::string node_n;
	static std::string metric;
	static std::string url_samples;
//	static size_t _timeAdjustment;
	static unsigned long long _startTime, _stopTime;
	static std::vector<long long> time_samples;  // vector will contain all time stamps from the specified interval 
	static std::vector<double>    power_samples; // vector will contain power samples corelated to timestamps 
#ifdef HAVE_RESTCLIENT_CPP_RESTCLIENT_H
	static RestClient::Connection* conn_samples;
#endif
};

////////////////////////////////////////////////////////////////////////////////

}



#endif /* SRC_WRAPPER_DAVIDEWRAPPER_H_ */
