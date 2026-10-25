#include "davidewrapper.h"

using namespace meric;

std::string DAVIDE::node_n;
std::string DAVIDE::metric;
unsigned long long DAVIDE::_startTime;
unsigned long long DAVIDE::_stopTime;
std::vector<long long> DAVIDE::time_samples; // vector will contain all time stamps from the specified interval 
std::vector<double>    DAVIDE::power_samples; // vector will contain power samples corelated to timestamps 
std::vector<std::string> DAVIDE::vrSensorLabel = {"blade"};
//const int DAVIDE::PROBE_NUMBER = vrSensorLabel.size() + 1; 
std::string DAVIDE::url_samples;
const std::string DAVIDE::SUM_ENERGY_LABEL = "Energy consumption [J]";

#ifdef HAVE_RESTCLIENT_CPP_RESTCLIENT_H
RestClient::Connection* DAVIDE::conn_samples;
void DAVIDE::init()
{
	std::string davideHostname = mutils::getHostnameShort();
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
	metric = "power";
	url_samples = std::string("http://") + std::string(KAIROSDB_SERVER) + std::string(":") + std::string(KAIROSDB_PORT) + std::string("/api/v1/") + std::string("datapoints/query");
	MERIC_INFO << "Hostname : " << node_n << "\n"
	           << "Server : " << KAIROSDB_SERVER << "\n"
	           << "URL is : " << url_samples << "\n";
	// initialize RestClient and get a connection object
	RestClient::init();
	conn_samples = new RestClient::Connection(url_samples);

	// configure basic auth
	conn_samples->SetBasicAuth(KAIROSDB_USER, KAIROSDB_PWD);

	// set connection timeout to 5s
	conn_samples->SetTimeout(30);
}

//TODO - missing error message return
std::string DAVIDE::getData(double startTime, double duration, SamplerRecord_t &record)
{
	MERIC_INFO << " ***************************************************************************\n";
	std::string errMessage = "";

	const double GPU_offset = 120.0;

	long long t0 = time_samples[0];
	long long ts = (long long)(FREQUENCY*startTime);
	long long dur_t = (long long)(FREQUENCY*duration);
	size_t firstBlade = (size_t)(ts - t0);
	size_t lastBlade = (size_t)((ts - t0) + dur_t);
	size_t n_samples = lastBlade - firstBlade;

	MERIC_INFO << "StartTime  : " << ts << "\n";
	if (dur_t == 0 || n_samples == 0)
	{
		MERIC_INFO << "Duration   : < 1 sample\n";
		firstBlade = -1;
		lastBlade = -1;
		dur_t = 0;
	}
	else
	{
		MERIC_INFO << "Duration   : " << dur_t << " samples\n";
	}
	MERIC_INFO << "firstBlade  : " << firstBlade << "\n"
			   << "lastBlade   : " << lastBlade << "\n"
			   << "# of Samples: " << n_samples << "\n";

	// sub-sampling measurement setup
	std::vector <double> tmp_t, tmp_w;
	int n_subsamples = dur_t*20;
	int hallo_size = 10;
	int buf_size = 2*hallo_size+dur_t+2;
	bool use_linear_interpolation = false; // if false spline interpolation is used 

	if (dur_t == 0)
		return errMessage;

	record.push_back(SamplerData_t(SourceInfo_t(SourceInfo_t::TYPE_POWER, SourceInfo_t::DOMAIN_PER_NODE, SourceInfo_t::FLAG_ACCUMULATIVE_BIT)));
	SamplerData_t &sensor = record.back();
	sensor.id = 0;
	sensor.name = vrSensorLabel[0];
	sensor.first = firstBlade;
	sensor.last = lastBlade;

	if (dur_t < 10 && firstBlade > hallo_size && time_samples.size() >= (firstBlade - hallo_size + buf_size - 1))
	{
		tmp_t.reserve(buf_size);
		tmp_w.reserve(buf_size);

		MERIC_INFO << "  Subsample energy consumption estimation:\n"
				   << "  StartTime   : " << (long long)(FREQUENCY * startTime * 1000.0) << "\n"
				   << "  StopTime    : " << (long long)(FREQUENCY * startTime * 1000.0 + FREQUENCY * duration * 1000.0) << "\n"
				   << "  Duration    : " << (long long)(FREQUENCY * duration * 1000.0) << " us\n"
				   << "  Start sample: " << firstBlade - hallo_size << std::endl
				   << "  Stop sample : " << firstBlade - hallo_size + buf_size - 1 << std::endl;

		for (size_t i = 0; i < buf_size; i++ )
		{
			tmp_t.push_back((double)time_samples[firstBlade - hallo_size + i]);
			tmp_w.push_back(power_samples[firstBlade - hallo_size + i]);

			MERIC_INFO << "    Time: " << time_samples[firstBlade - hallo_size + i] << "    [ms]; power: " << power_samples[firstBlade - hallo_size + i]
					   << " W \n";
		}

		tk::spline s;
		if (use_linear_interpolation)
		{
			s.set_points(tmp_t,tmp_w, false);  // void set_points(const std::vector<double>& x, const std::vector<double>& y, bool cubic_spline=true);
		}
		else
		{
			s.set_points(tmp_t,tmp_w, true); 
		}

		double inc = duration / double(n_subsamples); 

		MERIC_INFO << "    ------------------------\n";
		for (size_t i = 0; i < n_subsamples+1; i++)
		{
			double sub_time     = (startTime + (double)i * inc) * (double)FREQUENCY;
			double interp_power = s(sub_time);

			if (i == 0)
			{
				sensor.energy = interp_power;
				sensor.max = interp_power;
				sensor.min = interp_power;
			}
			else
			{
				sensor.max = std::max(sensor.max, interp_power) sensor.min = std::min(sensor.min, interp_power)

																				 sensor.energy += interp_power;
			}
 
			MERIC_INFO << "    Time: " << (long long) (sub_time*1000.0) <<  " [us]; power: " << interp_power << " W (sub-sample)\n"; 
		}
		MERIC_INFO << "    ------------------------\n";

		sensor.max -= GPU_offset;
		sensor.min -= GPU_offset;

		MERIC_INFO << "minWatts : " << sensor.min << " W\n"
				   << "maxWatts : " << sensor.max << " W\n";

		double avgWatts = sensor.energy / double(n_subsamples + 1);
		sensor.energy = (avgWatts - GPU_offset) * n_samples;
	}
	else
	{
		sensor.energy = 0.0;
		sensor.max = power_samples[firstBlade];
		sensor.min = power_samples[firstBlade];

		double valTmp = power_samples[firstBlade];
		for (size_t it = firstBlade; it < lastBlade; it++)
		{
			valTmp = power_samples[it];

			sensor.max = std::max(sensor.max, valTmp);
			sensor.min = std::min(sensor.min, valTmp);

			sensor.energy += valTmp;
		}

		sensor.max -= GPU_offset;
		sensor.min -= GPU_offset;

		MERIC_INFO << "minWatts : " << sensor.min << " W\n"
				   << "maxWatts : " << sensor.max << " W\n"
				   << "avgWatts : " << (sensor.energy / ((double)(n_samples))) - GPU_offset << " W (subtracted " << GPU_offset << "W for GPU idle (4x30W) \n";
		sensor.energy -= n_samples * GPU_offset;
	}

	return errMessage;
}

int DAVIDE::getSamples()
{
	MERIC_INFO << "Start time is   : "  << _startTime << " ms\n"
	           << "Stop time is    : "  << _stopTime << " ms\n"
	           << "Duration  is    : "  << _stopTime - _startTime << " ms\n";

	std::string stop_t  = std::to_string(_stopTime);    
	std::string start_t = std::to_string(_startTime);  

	std::string querry = "{\"cache_time\":0,\"end_absolute\":" + stop_t+ ",\"metrics\":[{\"group_by\":[{\"name\":\"tag\",\"tags\":[\"node\",\"cluster\",\"org\",\"plugin\"]}],\"name\":\""+metric+"\",\"tags\":{\"node\":[\""+node_n+"\"],\"plugin\":[\"bbb_pub\"],\"ts\":[\"1ms\"],\"chnl\":[\"data\"],\"cluster\":[\"davide\"],\"org\":[\"e4\"]}}],\"time_zone\":\"Europe/Rome\",\"start_absolute\":" +start_t+ "}";

	const int slt = 5;
	MERIC_INFO << "Waiting " << slt<<"s for KairosDB to get all samples." << "\n";		
	sleep(slt); //wait for smaples to be loaded into DB

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
		long long numOfSamples = std::stoll(std::string(((struct json_string_s*)data->value->payload)->string));
		if (numOfSamples == 0)
		{
			std::cerr << "MERIC ERROR: no power samples from DiG\n";
			return 1;
		}
		
		long long duration = _stopTime - _startTime;
		MERIC_INFO << "# of samples is : "  << numOfSamples << "\n"
		           << "# of lost smpls.: "  << duration - numOfSamples << "\n";

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


void DAVIDE::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> & samples)
{
	if (power_samples.size() > last)
		for (size_t i = first; i < last; i++)
			samples.push_back(power_samples[i]);
}

#endif

void DAVIDE::getSamplePeriodInfo(SamplePeriodInfo_t &info) { info = SamplePeriodInfo_t(); }

bool DAVIDE::isAvailable()
{
#ifdef HAVE_RESTCLIENT_CPP_RESTCLIENT_H
	return true;
#else
	return false;
#endif
}