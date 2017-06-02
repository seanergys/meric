

#include "jetsonwrapper.h"

using namespace meric;

std::string JETSON::filename="";
std::string JETSON::storeLabel="jetson";
std::vector<std::string> JETSON::vrSensorLabel = {"GPU", "CPU"}; //"DC",
std::vector<jetsonSample> JETSON::_samples;


void JETSON::init()
{
	//get name of the file with measurements
	char hostname[50];
	gethostname(hostname, 50);
	filename = std::string(JETSONMEASUREMENTPATH)+"/"+std::string(hostname)+".csv";
	
	struct stat buffer;   
	if (stat(filename.c_str(), &buffer) != 0)
	{
		std::cerr << "MERIC ERROR: jetson measurement file "+filename+" doesn't exist\n";
		//exit
	}
}


int JETSON::getSamples()					//prejmenovat loadSamples()
{
	std::ifstream iFile(filename);
	if(iFile.is_open())
	{
		int cnt = 0;
		std::string data;
		std::getline(iFile, data);
		jetsonSample sample;
		int state = 0;
		while(iFile >> data)
		{
			switch(state)
			{
				case 0 : 
				{
					sample.timestamp = mutils::timeToDouble(mutils::stringToTime(data));
					state++;
					break;
				}
				case 1 :
				{
					sample.DC = std::stod(data);
					state++;
					break;
				}
				case 2 :
				{
					sample.GPU = std::stod(data);
					state++;
					break;
				}
				case 3 :
				{
					sample.CPU = std::stod(data);
					_samples.push_back(sample);
					state=0;
//	MERIC_INFO << "T: "<<int(sample.timestamp) <<"\t("<<++cnt<<")\tE: "<<sample.DC << "\n";
					break;
				}
			}
		}
		iFile.close();
	}
	else
	{
		std::cerr << "MERIC ERROR: jetson measurement file cannot be open\n";
	}

	MERIC_INFO <<"JETSON samples #" << _samples.size() << std::endl;
	if (_samples.size() == 0)
	{
		std::cerr << "MERIC ERROR: no JETSON samples\n";
		return 1;
	}
	return 0;
}

std::string JETSON::getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double energy[], double minWatts[], double maxWatts[])	//TODO getRegionData
{
	*firstBlade = 0;
	*lastBlade  = 0;
	*energy     = 0.0;
	*minWatts   = 0.0;
	*maxWatts   = 0.0;

	if (_samples.size() == 0)
		return "";

	*firstBlade = std::ceil((startTime - _samples[0].timestamp) * FREQUENCY);
	*firstBlade += _samples[*firstBlade].timestamp < startTime ? 1 : 0;
	*lastBlade  = std::floor(duration * FREQUENCY) + *firstBlade;

	if (*lastBlade > _samples.size())
	{
		*lastBlade = -1;
		return "MERIC ERROR: missing JETSON samples for region ";
	}
	
	*firstVR = *firstBlade;
	*lastVR  = *lastBlade;

	if(*lastBlade > 0 && *lastBlade > *firstBlade)
	{
/*
		*maxWatts = _samples[*firstBlade].DC;
		*minWatts = *maxWatts;
		for (size_t it = *firstBlade; it < *lastBlade; it++)
		{
			double valTmp = _samples[it].DC;
			if (valTmp > *maxWatts)
				*maxWatts = valTmp;
			else if (valTmp < *minWatts)
				*minWatts = valTmp;
			*energy += valTmp;
		}
*/		

/**/
			for (size_t it = *firstBlade; it < *lastBlade; it++)						//TODO
			{
				for (size_t sensorId = 0; sensorId < JETSON::PROBE_NUMBER - 1; sensorId++)
				{
					double valTmp = 0.0;
					switch(sensorId)
					{
						case 0 :
						{
							valTmp = _samples[it].DC;
							break;
						}
						case 1 :
						{
							valTmp = _samples[it].GPU;
							break;
						}
						case 2 :
						{
							valTmp = _samples[it].CPU;
							break;
						}
					}
					
					if (valTmp > maxWatts[sensorId])	//max[]
						maxWatts[sensorId] = valTmp;	//max[]
					else if (valTmp < minWatts[sensorId])	//min[]
						minWatts[sensorId] = valTmp;	//min[]
					energy[sensorId] += valTmp;	//energy[sensorId]
				}
			}
/**/
		
	}else
	{
		*firstBlade = -1;
		*lastBlade  = -1;
		return "MERIC ERROR: No JETSON samples, probably too short ("+std::to_string(duration)+"s) region ";
	}
	
	return "";
}


void JETSON::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> samples)
{
	switch(sensorId)
	{
		case 0 :
		{
			for (size_t i = first; i < last; i++)
				samples.push_back(_samples[i].DC);
			break;
		}
		case 1 :
		{
			for (size_t i = first; i < last; i++)
				samples.push_back(_samples[i].GPU);
			break;
		}
		case 2 :
		{
			for (size_t i = first; i < last; i++)
				samples.push_back(_samples[i].CPU);
			break;
		}
	}
}





