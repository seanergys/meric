

#include "jetsonwrapper.h"

using namespace meric;

std::string JETSON::filename="";
std::string JETSON::storeLabel="jetson";
std::vector<std::string> JETSON::vrSensorLabel = {"GPU", "CPU"}; //"DC",
std::vector<jetsonSample> JETSON::_samples;
int JETSON::FREQUENCY = 10;


void JETSON::init() //int rank = 0)
{
	//start energy measurement
	system((std::string(JETSONMEASUREMENTPATH)+"/getJTX1measurements.py &").c_str());
	
	//get name of the file with measurements
	char hostname[50];
	gethostname(hostname, 50);
	filename = std::string(JETSONMEASUREMENTPATH)+"/jetsonMeasurement/"+std::string(hostname)+".csv";
}


int JETSON::getSamples()					//prejmenovat loadSamples()
{
	//wait to load all samples
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	double currentTime = mutils::timeToDouble(time);
	double timestamp;
	do
	{
		sleep(2);
		timestamp = 0.0;
		std::ifstream tFile(filename);
		if(tFile.is_open())
		{		
			tFile.seekg(-2,std::ios_base::end); // seek before the EOF
			bool keepLooping = true;
			while(keepLooping)
			{
				char ch;
				tFile.get(ch);

				if((int)tFile.tellg() <= 1) // the first line is the last line
				{
					tFile.seekg(0);
					keepLooping = false;
				}
				else if(ch == '\n')
					keepLooping = false;
				else
					tFile.seekg(-2,std::ios_base::cur);
			}

			std::string lastLine;            
			getline(tFile,lastLine);
			size_t space = lastLine.find(',');
			if (space != std::string::npos)
			{
				timestamp = mutils::timeToDouble(mutils::stringToTime(lastLine.substr(0,space)));
				//MERIC_INFO << currentTime << " vs " << timestamp << std::endl;
			}
			tFile.close();
		}
	}while(timestamp <= currentTime);
	
	//stop energy measurement
	system("pkill -f -INT getJTX1measurements.py");
	
	//load samples
	std::ifstream iFile(filename);
	if(iFile.is_open())
	{
		int cnt = 0;
		std::string data;
		std::getline(iFile, data);	//header
	
		jetsonSample sample;
		int state = 0;
		while(iFile >> data)
		{
			switch(state)
			{
				case 0 : 
				{
					sample.timestamp = std::stod(data);
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

	//stats
	MERIC_INFO <<"JETSON samples #" << _samples.size() << std::endl;
	MERIC_INFO <<std::setprecision (15) <<"JETSON RUNTIME: " << _samples[0].timestamp << " - " << _samples[_samples.size()-1].timestamp << std::endl;
	if (_samples.size() == 0)
	{
		std::cerr << "MERIC ERROR: no JETSON samples\n";
		return 1;
	}
	FREQUENCY = std::ceil(_samples.size() / (_samples[_samples.size()-1].timestamp-_samples[0].timestamp));
	MERIC_INFO << "JETSON samples frequency: " << FREQUENCY << std::endl;
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
	if (_samples.size() <= *firstBlade)
		*firstBlade = _samples.size()-1;
	while (_samples[*firstBlade].timestamp > startTime)
		*firstBlade -= std::ceil(_samples[*firstBlade].timestamp - startTime);
	while (_samples[*firstBlade].timestamp < startTime)
		*firstBlade += 1;
	*lastBlade  = std::floor(duration * FREQUENCY) + *firstBlade;
	const double endTime = startTime + duration;
	while (_samples[*lastBlade].timestamp > endTime)
		*lastBlade -= std::ceil(_samples[*lastBlade].timestamp - endTime);
	while (_samples[*lastBlade].timestamp < endTime)
		*lastBlade += 1;

	if (*lastBlade > _samples.size())
	{
		*lastBlade = -1;
		return "MERIC ERROR: missing JETSON samples for region ";
	}
	
	*firstVR = *firstBlade;
	*lastVR  = *lastBlade;

	if(*lastBlade > 0 && *lastBlade > *firstBlade)
	{
		for (size_t it = *firstBlade; it < *lastBlade; it++)
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
				
				if (valTmp > maxWatts[sensorId])
					maxWatts[sensorId] = valTmp;
				else if (valTmp < minWatts[sensorId])
					minWatts[sensorId] = valTmp;
				energy[sensorId] += valTmp;
			}
		}
	}else
	{
		*firstBlade = -1;
		*lastBlade  = -1;
		*firstVR    = -1;
		*lastVR     = -1;
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





