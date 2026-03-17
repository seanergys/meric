#include "occwrapper.h"
#include "environmentwrapper.h"

using namespace meric;

bool OCC::detailedMode = false;
std::string OCC::suffix = "_";
int OCC::OCC_fd = -1;
std::map<std::string, uint64_t> OCC::counters;

/**
 * initialization of the OCC class
 * opens file descriptor, from which the counters are read
 * in detailed mode each power sensor is reported separately
 **/
void OCC::init(bool detailed)
{
	OCC_fd = open(OCC_INBAND_SENSORS, O_RDONLY);
	if (OCC_fd < 0)
	{
		std::cerr << "MERIC ERROR: failed to open OCC file descriptor\n";
		return;
	}
	detailedMode = detailed;
	initMap(counters);
}

/**
 * returns result value of the counter (std::string) based on its startValue and
 * stopValue and time between these two measurements
 * handles counter overflow
 **/
uint64_t OCC::getResultValue(uint64_t startValue, uint64_t stopValue, double runtime, int & cumulative, std::string counter, std::string regionName)
{
	cumulative = 0;
	std::string counterName;
	if (detailedMode && counter != "PWRSYS")
		counterName = counter.substr(0, counter.find_last_of('_'));
	else
		counterName = counter;
	
	if (stopValue < startValue)
	{	
		MERIC_INFO << "OCC counter OVERFLOW, MAX VALUE: " <<UINT64_MAX <<" " <<counterName << std::endl;
		MERIC_INFO << "OCC counter REPAIR " << startValue << " .. " << stopValue << " = " << ((UINT64_MAX - startValue) + stopValue) << std::endl;
		return ((UINT64_MAX - startValue) + stopValue);
	}
	else
	{
		return (stopValue - startValue);
	}
}

/**
 * read values of the counters (std::string) from the input dictionary and store
 * the values into the dictionary (uint64_t)
 **/
void OCC::getCounterValues(std::map<std::string, uint64_t> & record)
{
	for(std::map<std::string, uint64_t>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
	{
		record[iterator->first] = 0;
	}
	
	for (int die=0; die<Environment::SOCKETS_PER_NODE; die++)
	{
		//read the values
		void *occ_buff[OCC_SENSOR_DATA_BLOCK_SIZE];
		int rc, bytes;
		for(rc = bytes = 0; bytes < OCC_SENSOR_DATA_BLOCK_SIZE; bytes += rc) 
		{
			rc = read(OCC_fd, occ_buff + bytes, OCC_SENSOR_DATA_BLOCK_SIZE - bytes);
			if(!rc || rc < 0)
				break;
		}
		
		//store the values
		const int suffixLength = log10(Environment::SOCKETS_PER_NODE)+1;
		std::string counterSuffix;
		if (detailedMode)
		{
			std::ostringstream os;
			os << std::setfill('0') << std::setw(suffixLength) << die;
			counterSuffix = suffix + os.str();
		}
		
		occ_sensor_data_header_t *hb = (occ_sensor_data_header_t *)(uint64_t)occ_buff;
		occ_sensor_name_t *md = (occ_sensor_name_t *)((uint64_t)hb + be32toh(hb->names_offset));

		for(int j = 0; j < be16toh(hb->nr_sensors); j++)
		{
			uint32_t offset = be32toh(md[j].reading_offset);
			if(be16toh(md[j].type) == OCC_SENSOR_TYPE_POWER)
			{
				const uint8_t * ping = (uint8_t *)((uint64_t)hb + be32toh(hb->reading_ping_offset));
				const occ_sensor_record_t * sensor_data = (occ_sensor_record_t *)((uint64_t)ping + offset);
				const uint32_t sensor_freq = be32toh(md[j].freq);

				std::string sensorName(md[j].name);
				if (detailedMode && sensorName != "PWRSYS")
					sensorName += counterSuffix;
				if (record.count(sensorName))
					record[sensorName] += (uint64_t)(be64toh(sensor_data->accumulator) / TO_FP(sensor_freq));
			}
		}
	}
	//reset file offset
	if(lseek(OCC_fd, 0, SEEK_SET) < 0)
	{
		::close(OCC_fd);
		OCC_fd = open(OCC_INBAND_SENSORS, O_RDONLY);
		if (OCC_fd < 0)
		{
			std::cerr << "ERROR: failed to re-open OCC file descriptor\n";
			return;
		}
	}
}

/**
 * each counter has a label, that will represent this counter in the output file
 * returns a label of the input counter
 **/
std::string OCC::getCounterLabel(std::string counter)
{
	std::map<const std::string, std::string>::iterator it = counterLabel.find(counter);
	if (it != counterLabel.end())
		return it->second;
	else
		return counter;
}

/**
 * fill-in the RAPL counters into the dictionary, that will keep values of the required counters
 **/
void OCC::initMap(std::map<std::string, uint64_t> & dict)
{
// fill this vector
	std::vector <std::string> names;
	names.push_back("PWRPROC");
	names.push_back("PWRMEM");
	names.push_back("PWRGPU");

// DO NOT EDIT THE REST OF THIS FUNCTION
	if (detailedMode)
	{
		const int suffixLength = log10(Environment::SOCKETS_PER_NODE)+1;
		for(int die = 0; die != Environment::SOCKETS_PER_NODE; die++)
		{
			for(std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it)
			{
				std::ostringstream os;
				os << std::setfill('0') << std::setw(suffixLength) << die;
				const std::string counterName = *it + suffix + os.str();
				dict[counterName] = 0;
				
				counterLabel[counterName] = counterLabel[*it].substr(0,counterLabel[*it].find(' '))+suffix + os.str() + counterLabel[*it].substr(counterLabel[*it].find(' '));
			}
		}
	}else
	{
		for(std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it)
		{
			dict[*it] = 0;
		}
	}
	
	//single counter per node, its overal energy consumption
	dict["PWRSYS"] = 0;
}

bool OCC::isAvailable()
{
#ifdef HAVE_OCC
	return true;
#endif
	return false;
}