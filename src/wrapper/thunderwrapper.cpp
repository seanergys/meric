

#include "thunderwrapper.h"

using namespace meric;

std::string THUNDER::filename="";
std::vector<std::string> THUNDER::vrSensorLabel = {}; //"DC",
std::vector<thunderSample> THUNDER::_samples;
int THUNDER::FREQUENCY = 1;
const std::string THUNDER::SUM_ENERGY_LABEL = "Energy consumption [J]";


void THUNDER::init() //int rank = 0)
{
	//start energy measurement
	if (system(("ssh -f hca \""+std::string(MERIC_PATH)+"/tools/getYokogawaMeasurements.py "+ARM_INFO+"\"").c_str()))
		std::cerr << "THUNDER energy measurement system ERROR: connection failed\n";
	filename = std::string(MERIC_PATH)+"/tools/yokogawaMeasurement/hca-server.csv"; //one file for all nodes
}


int THUNDER::getSamples()					//prejmenovat loadSamples()
{
	//wait to load all samples
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	double currentTime = mutils::timeToDouble(time);
	//stop energy measurement
	sleep(5); //output file is located at hca-server and it will be moved to the system when the measurement is closed
	if (system("ssh hca \"pkill -f -INT getYokogawaMeasurements.py\""))
		std::cerr << "THUNDER energy measurement system ERROR: connection failed\n";
	sleep(1);
	
	//load samples
	std::ifstream iFile(filename);
	if(iFile.is_open())
	{
		std::string data;
		std::getline(iFile, data);	//header
	
		thunderSample sample;
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
					_samples.push_back(sample);
					state = 0;
					break;
				}
			}
		}
		iFile.close();
	}
	else
	{
		std::cerr << "MERIC ERROR: ARM measurement file cannot be open\n";
	}

	//stats
	MERIC_INFO <<"ARM samples #" << _samples.size() << std::endl;
	if (_samples.size() == 0)
	{
		std::cerr << "MERIC ERROR: no ARM samples\n";
		return 1;
	}
	MERIC_INFO <<std::setprecision (15) <<"ARM RUNTIME: " << _samples[0].timestamp << " - " << _samples[_samples.size()-1].timestamp << std::endl;
	FREQUENCY = std::ceil(_samples.size() / (_samples[_samples.size()-1].timestamp-_samples[0].timestamp));
	MERIC_INFO << "ARM samples frequency: " << FREQUENCY << std::endl;
	return 0;
}

std::string THUNDER::getData(double startTime, double duration, SamplerRecord_t &record)
{
	if (_samples.size() == 0)
		return "";

	size_t firstBlade = std::ceil((startTime - _samples[0].timestamp) * FREQUENCY);
	if (_samples.size() <= firstBlade)
		firstBlade = _samples.size()-1;
	while (_samples[firstBlade].timestamp > startTime)
	{
		firstBlade -= std::ceil(_samples[firstBlade].timestamp - startTime);
		if (firstBlade <= 0)
		{
			firstBlade = 0;
			break;
		}
	}
	while (_samples[firstBlade].timestamp < startTime)
		firstBlade += 1;
	size_t lastBlade  = std::floor(duration * FREQUENCY) + firstBlade;
	if (lastBlade > _samples.size())
		lastBlade = _samples.size()-1;
	const double endTime = startTime + duration;
	while (_samples[lastBlade].timestamp > endTime)
		lastBlade -= std::ceil(_samples[lastBlade].timestamp - endTime) * FREQUENCY;
	if (lastBlade < firstBlade)
		lastBlade = firstBlade;
	while (lastBlade < _samples.size() && _samples[lastBlade].timestamp < endTime)
		lastBlade += 1;

	if (lastBlade == _samples.size())
	{
		lastBlade = std::numeric_limits<size_t>::max();
		return "MERIC ERROR: missing ARM samples for region ";
	}

	record.push_back(SamplerData_t(SourceInfo_t(SourceInfo_t::TYPE_POWER, SourceInfo_t::DOMAIN_PER_NODE, SourceInfo_t::FLAG_ACCUMULATIVE_BIT)));
	SamplerData_t &sensor = record.back();

	sensor.id = 0;
	sensor.name = vrSensorLabel[0];
	sensor.first = firstBlade;
	sensor.last  = lastBlade;

	if(lastBlade > 0 && lastBlade > firstBlade)
	{
		for (size_t it = firstBlade; it < lastBlade; it++)
		{
			double valTmp = _samples[it].DC;
			
			if (valTmp > sensor.max)
				sensor.max = valTmp;
			else if (valTmp < sensor.min)
				sensor.min = valTmp;
			sensor.energy += valTmp;
		}
	}else
	{
		/*
		*firstBlade = std::numeric_limits<size_t>::max();
		*lastBlade  = std::numeric_limits<size_t>::max();
		*/
		return "MERIC ERROR: No ARM samples, probably too short ("+std::to_string(duration)+"s) region ";
	}
	
	return "";
}


void THUNDER::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> samples)
{
	for (size_t i = first; i < last; i++)
		samples.push_back(_samples[i].DC);
}

void THUNDER::getSamplePeriodInfo(SamplePeriodInfo_t &info)
{
	uint64_t period = 1000000000ull / FREQUENCY;
	info = SamplePeriodInfo_t(period, period, period);
}

bool THUNDER::isAvailable()
{
	return true;
}