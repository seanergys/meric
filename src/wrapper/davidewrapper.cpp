
#include "../basis/spline.h"
#include "davidewrapper.h"

using namespace meric;

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

#ifdef HAVE_RESTCLIENT_CPP_RESTCLIENT_H
RestClient::Connection* DAVIDE::conn_samples;
void DAVIDE::init()
{

	char hostname[50];
   	gethostname(hostname, 50);
   	std::string davideHostname = std::string(hostname);
   	std::string::iterator it = davideHostname.begin();
	while (it != davideHostname.end() && !std::isdigit(*it))
		++it;
	if (!davideHostname.empty() || it != davideHostname.end())
	{
		int numero;
		std::stringstream(std::string(it, davideHostname.end())) >> numero;
		davideHostname = std::to_string(numero);
	}
	else
		davideHostname = "";
	node_n = "davide" + davideHostname;
	
	//MERIC_INFO << "Using hardcoded machine name due to problems with hostnames on DAVIDE\n"; 
	//node_n = std::string(hostname); //"davide43";
	//node_n = std::string("davide1");
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
	MERIC_INFO << " ***************************************************************************\n";
	std::string errMessage = "";

	double GPU_offset = 120.0; 

	long long t0 = time_samples[0]; 
	long long ts = (long long)(FREQUENCY*startTime);
	long long dur_t = (long long)(FREQUENCY*duration);
	*firstBlade = (size_t)(ts - t0); 
	*lastBlade = (size_t)((ts - t0) + dur_t);  
	size_t n_samples = *lastBlade - *firstBlade;


	MERIC_INFO << "StartTime  : " << ts << "\n";
	if (dur_t == 0) {	
		MERIC_INFO << "Duration   : < 1 sample\n";
	} else {
		MERIC_INFO << "Duration   : " << dur_t << " samples\n";
	}
	MERIC_INFO << "firstBlade : " << *firstBlade << "\n";
	MERIC_INFO << "lastBlade  : " << *lastBlade  << "\n";
	MERIC_INFO << "# of Samples:" << n_samples << "\n";

	// sub-sample measurement 
	
	std::vector <double> tmp_t, tmp_w;

	if (dur_t < 10) {	

		// Sub sampling setup
		int  n_subsamples 			  = dur_t*20;
		int  hallo_size   			  = 10;  
		bool use_linear_interpolation = false; // if false spline interpolation is used 
		// END - Sub sampling setup


		MERIC_INFO << "  Subsample energy consumption estimation:\n";
		MERIC_INFO << "  StartTime  : " << (long long)(FREQUENCY*startTime*1000.0) << "\n";
		MERIC_INFO << "  StopTime   : " << (long long)(FREQUENCY*startTime*1000.0 + FREQUENCY*duration*1000.0) << "\n";
		MERIC_INFO << "  Duration   : " << (long long)(FREQUENCY*duration*1000.0) << " us\n";

		int  buf_size = 2*hallo_size+dur_t+2;
		tmp_t.reserve(buf_size); 
		tmp_w.reserve(buf_size);

		for (size_t i = 0; i < buf_size; i++ ) {
			tmp_t.push_back((double)  time_samples [*firstBlade - hallo_size + i]); 
			tmp_w.push_back(          power_samples[*firstBlade - hallo_size + i]);

			MERIC_INFO << "    Time: " << time_samples [*firstBlade - hallo_size + i] << "    [ms]; power: " 
			                       << power_samples[*firstBlade - hallo_size + i] << " W \n";
		}

		tk::spline s;

		if (use_linear_interpolation) {
			s.set_points(tmp_t,tmp_w, false);  // void set_points(const std::vector<double>& x, const std::vector<double>& y, bool cubic_spline=true);
		} else {
			s.set_points(tmp_t,tmp_w, true); 
		}

		double inc = duration / double(n_subsamples); 

		MERIC_INFO << "    ------------------------\n";
		for (size_t i = 0; i < n_subsamples+1; i++) {
			
			double sub_time     = (startTime + (double)i * inc ) * (double)FREQUENCY;
			double interp_power = s(sub_time);

			if (i == 0) {
				energy[0]   = interp_power;
				maxWatts[0] = interp_power;
				minWatts[0] = interp_power;
			} else {

				if (interp_power > maxWatts[0])
					maxWatts[0] = interp_power;
			
				if (interp_power < minWatts[0])
					minWatts[0] = interp_power; 

				energy[0] += interp_power;
			}
 
			MERIC_INFO << "    Time: " << (long long) (sub_time*1000.0) <<  " [us]; power: " << interp_power << " W (sub-sample)\n"; 
		}
		MERIC_INFO << "    ------------------------\n";

		maxWatts[0]-= GPU_offset;
		minWatts[0]-= GPU_offset;

		MERIC_INFO << "minWatts : " << minWatts[0] << " W\n";
		MERIC_INFO << "maxWatts : " << maxWatts[0] << " W\n";

		double avgWatts = (energy[0] / double(n_subsamples+1)) - GPU_offset;
		MERIC_INFO << "avgWatts : " << avgWatts << " W (subtracted " << GPU_offset << "W for GPU idle (4x30W)) \n";	
		energy[0] = avgWatts * duration * 1000.0; // the * 1000.0 is for mW to W conversion from hdeem
		MERIC_INFO << "energy   : " << avgWatts * duration << " J\n";		

	} else {

		energy[0]   = 0.0;
		maxWatts[0] = power_samples[*firstBlade];
		minWatts[0] = power_samples[*firstBlade];

		double valTmp = power_samples[*firstBlade];
		for (size_t it=*firstBlade; it <*lastBlade; it++)
		{
			valTmp = power_samples[it];

			if (valTmp > maxWatts[0])
				maxWatts[0] = valTmp;
			
			if (valTmp < minWatts[0]) //&& valTmp > 0.0)
				minWatts[0] = valTmp;
			
			energy[0] += valTmp;
		}

		maxWatts[0]-= GPU_offset;
		minWatts[0]-= GPU_offset;

		MERIC_INFO << "minWatts : " << minWatts[0] << " W\n";
		MERIC_INFO << "maxWatts : " << maxWatts[0] << " W\n";

		MERIC_INFO << "avgWatts : " << ( energy[0] / ((double)(n_samples)) ) - GPU_offset << " W (subtracted 120W for GPU idle (4x30W) \n";	
		energy[0] = ((energy[0] / ((double)(n_samples)) ) - GPU_offset ) * duration * 1000.0; // the * 1000.0 is for MW to W conversion from hdeem
		MERIC_INFO << "energy   : " << energy[0] / 1000.0 << " J\n";

	}	

	return errMessage;	
}

int DAVIDE::getSamples()
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

	const char * json_data = r.body.c_str();
	struct json_value_s* root = json_parse(json_data, r.body.size());
	if (root ==  NULL)
	{
		std::cerr << "MERIC ERROR: empty or invalid json from DiG\n";
		return 1;
	}	
	struct json_object_element_s* data = ((struct json_object_s*)root->payload)->start;
	if (data != NULL && std::string(data->name->string) == "queries")
	{
		struct json_array_element_s* arr = ((struct json_array_s*)data->value->payload)->start;
		data = ((struct json_object_s*)arr->value->payload)->start;
		long long numOfSamples = std::stoi(std::string(((struct json_string_s*)data->value->payload)->string));
		if (numOfSamples == 0)
		{
			std::cerr << "MERIC ERROR: no power samples from DiG\n";
			return 1;
		}
		
		long long duration = _stopTime - _startTime;
		MERIC_INFO << "# of samples is : "  << numOfSamples << "\n";
		MERIC_INFO << "# of lost smpls.: "  << duration - numOfSamples << "\n";

		time_samples.resize (duration + 1);
		power_samples.resize(duration + 1);

		//std::cerr << ((struct json_string_s*)data->value->payload)->string << std::endl;
		data = data->next;
		arr = ((struct json_array_s*)data->value->payload)->start;
		data = ((struct json_object_s*)arr->value->payload)->start;

		while(data != NULL && std::string(data->name->string) != "values")
			data = data->next;
		if (data != NULL)
		{

			auto getSingleSample = [&] (struct json_array_element_s* arr, long long *time, double *power)
			{
				if (arr != NULL)
				{
					struct json_array_element_s* sample = ((struct json_array_s*)arr->value->payload)->start;
					*time = std::stoll(std::string(((struct json_string_s*)sample->value->payload)->string));
					sample = sample->next;
					*power = std::stod(std::string(((struct json_string_s*)sample->value->payload)->string));
				}
			};

			arr = ((struct json_array_s*)data->value->payload)->start; //array of time-power samples
			long long it  = 0; 
			long long it2 = 0;
			int missing_samples = 0; 
			int last_error_index = 0;
			long long time = 0;
			double power = .0;
			getSingleSample(arr, &time, &power);
			double corrW = power;
			for (long long ts = _startTime; ts <= _stopTime; ts++)
			{
				time_samples[it2] = ts;
				if (time == ts && it < numOfSamples)
				{
					power_samples[it2] = power;
					corrW = power;
					if (it2 < 20 || it2 > (duration - 20))
						MERIC_INFO << "Index : " << it2 << " - " << it << "\tTime s: " << ts << " ms\tPower s: " << power_samples[it2] << " W \n";
					it++;
					arr = arr->next;
					getSingleSample(arr, &time, &power);
				}
				else
				{
					power_samples[it2] = corrW;
					missing_samples++;
					if (it2 < 20 || it2 > (duration - 20))
						MERIC_INFO << "Index : " << it2 << " - " << it << "\tTime s: " << ts << " ms\tPower s: " << power_samples[it2] << " W ----- corrected value ---- last error : " << it2 - last_error_index << " \n";
					last_error_index = it2;
				}
				it2++;
			}
		}
		else
		{
			std::cerr << "MERIC ERROR: invalid json from DiG\n";
			free(root);
			return 1;
		}
	}
	else
	{
		std::cerr << "MERIC ERROR: empty or invalid json from DiG\n";
		free(root);
		return 1;
	}
	return 0;
}


//sensorId bude 0 - blade
void DAVIDE::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> & samples)
{
	if (power_samples.size() > last)
		for (size_t i = first; i < last; i++)
			samples.push_back(power_samples[i]);
}

#endif







