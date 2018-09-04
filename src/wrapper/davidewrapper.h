
#ifndef SRC_WRAPPER_DAVIDEWRAPPER_H_
#define SRC_WRAPPER_DAVIDEWRAPPER_H_

#include "../basis/utils.h"
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <ctime>
#include <array>
#include <vector>
#include <map>
#include <unistd.h>
#include <cmath>
#include <climits>


// *** KairosDB setup for DAVIDE ponitoring system 
#define KAIROSDB_SERVER "192.168.1.97"
#define KAIROSDB_PORT   "8083"
#define KAIROSDB_USER   "admin"
#define KAIROSDB_PWD    "k41r0sdb768"

// *** Rest client headers required to access KairosDD
#include "restclient-cpp/connection.h"
#include "restclient-cpp/restclient.h"

// *** JSON single header library used to parse JSON quieries 
#include <nlohmann/json.hpp>
using json = nlohmann::json;


namespace meric {

class DAVIDE {

public:

	static const int FREQUENCY = 1000;
	static const int PROBE_NUMBER = 1;	//BLADE + {"CPU0", "CPU1", "DDR AB", "DDR CD", "DDR EF", "DDR GH"}; kolik zarizeni merime
	static std::string storeLabel;
	static std::vector<std::string> vrSensorLabel;	//Voltage regulator

	static void init();
/*	{
		node_n = "davide43"; 
		metric = "power";
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
*/	



	static void close() { RestClient::disable(); }

	// long long TimeSpecToMiliSec(struct timespec time) {
		// return time.tv_sec * 1000.0 + time.tv_nsec / 1000000.0;
	// }

	static void start(timespec & time)
	{
		clock_gettime(CLOCK_REALTIME, &time);
		_startTime =  time.tv_sec * 1000.0 + time.tv_nsec / 1000000.0; //(long long) mutils::timeToDouble(time)*1000.0;
		MERIC_INFO << "Setting start time \n";
	}
	
	static void stop(timespec & time)
	{
		clock_gettime(CLOCK_REALTIME, &time);
		_stopTime = time.tv_sec * 1000.0 + time.tv_nsec / 1000000.0; //(long long)mutils::timeToDouble(time)*1000.0;
		MERIC_INFO << "Setting stop time \n";
	}
	static void setTimeAdjustment(timespec start) {};
	
	static int getSamples()
	{
		
		MERIC_INFO << "Start time is   : "  << _startTime << " ms\n";
		MERIC_INFO << "Stop time is    : "  << _stopTime << " ms\n";
		MERIC_INFO << "Duration  is    : "  << _stopTime - _startTime << " ms\n";


		std::string stop_t  = std::to_string(_stopTime);    
		std::string start_t = std::to_string(_startTime);  

		std::string querry = "{\"cache_time\":0,\"end_absolute\":" + stop_t+ ",\"metrics\":[{\"group_by\":[{\"name\":\"tag\",\"tags\":[\"node\",\"cluster\",\"org\",\"plugin\"]}],\"name\":\""+metric+"\",\"tags\":{\"node\":[\""+node_n+"\"],\"plugin\":[\"bbb_pub\"],\"ts\":[\"1ms\"],\"chnl\":[\"data\"],\"cluster\":[\"davide\"],\"org\":[\"e4\"]}}],\"time_zone\":\"Europe/Rome\",\"start_absolute\":" +start_t+ "}";

		int slt = 5; 
		MERIC_INFO << "Waiting " << slt<<"s for KairosDB to get all samples." << "\n";		
		sleep(slt); //wait for smaples to be loaded into DB
		//MERIC_INFO << "Querry is : "  << querry << "\n";

		RestClient::Response r = conn_samples->post("", querry);

		//MERIC_INFO << "Server message code: "    << r.code     << "\n\n"; 
		//MERIC_INFO << "Server message body: "    << r.body     << "\n\n"; 

		json j = json::parse(r.body);
		json v = j["/queries/0/results/0/values"_json_pointer];

		//MERIC_INFO << "\nValues are:\n" << v.dump(4) << "\n\n";

		// *** array parsing ***************************************************
		// std::vector<long long> time_samples  (v.size(),0);
		// std::vector<double>    power_samples (v.size(),0);	
		
		long long duration = _stopTime - _startTime;
		MERIC_INFO << "# of samples is : "  << v.size() << "\n";
		MERIC_INFO << "# of lost smpls.: "  << duration - v.size() << "\n";

		time_samples.resize (duration + 1);
		power_samples.resize(duration + 1);

		long long it  = 0; 
		long long it2 = 0;
		double corrW = (double) v[0][1];
		int missing_samples = 0; 
		int last_error_index = 0; 
		for (long long ts = _startTime; ts <= _stopTime; ts++)
		{
			time_samples [it2] = ts;
			if ((long long) v[it ][0] == ts && it < v.size() ) {
				power_samples[it2]     = (double)    v[it ][1];
				corrW			       = power_samples[it2];
				if (it2 < 20 || it2 > (duration - 20)) {
					MERIC_INFO << "Index : " << it2 << " - " << it << "\tTime s: " << ts << " ms\tPower s: " << power_samples[it2] << " W \n";
				}
				it++;
			} else {
				power_samples[it2] = corrW;
				missing_samples++; 
				if (it2 < 20 || it2 > (duration - 20)) { 
					MERIC_INFO << "Index : " << it2 << " - " << it << "\tTime s: " << ts << " ms\tPower s: " << power_samples[it2] << " W ----- corrected value ---- last error : " << it2 - last_error_index << " \n";
				}
				last_error_index = it2; 
			}
			it2++;  
		}
		// *** END - array parsing *********************************************
		return 0;
	}
	static void deleteSamples() {};
	

	static std::string getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double energy[], double min[], double max[]);
	static void getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> & samples);
	static std::string getCountersLabel() {	return "DAVIDE";};

private:
	static std::string node_n;
	static std::string metric;
	static std::string url_samples;
	static RestClient::Connection* conn_samples;
//	static size_t _timeAdjustment;
	static unsigned long long _startTime, _stopTime;
	static std::vector<long long> time_samples;  // vector will contain all time stamps from the specified interval 
	static std::vector<double>    power_samples; // vector will contain power samples corelated to timestamps 

};

////////////////////////////////////////////////////////////////////////////////

}



#endif /* SRC_WRAPPER_DAVIDEWRAPPER_H_ */
