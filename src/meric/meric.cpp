
#include "meric.h"

using namespace meric;

int         Meric::MODE       = mutils::getenv<int>("MERIC_MODE" , MODE_HDEEM);
bool        Meric::CONTINUAL  = mutils::getenv("MERIC_CONTINUAL" , false);
bool        Meric::DETAILED   = mutils::getenv("MERIC_DETAILED"  , false);
bool        Meric::AGGREGATE  = mutils::getenv("MERIC_AGGREGATE" , false);
bool        Meric::DEBUG      = mutils::getenv("MERIC_DEBUG"     , false);
bool        Meric::SAMPLES    = mutils::getenv("MERIC_SAMPLES"   , false);
std::string Meric::COUNTERS   = mutils::getenv("MERIC_COUNTERS"  , std::string("none"));
std::string Meric::OUTPUT_DIR = mutils::getenv("MERIC_OUTPUT_DIR", std::string("hdeemMeasurement"));
std::string Meric::OUTPUT_FILENAME = mutils::getenv("MERIC_OUTPUT_FILENAME", std::string("log"));
std::string Meric::REGION_OPTIONS  = mutils::getenv("MERIC_REGION_OPTIONS", std::string(""));

std::map<std::string, std::map<std::pair<int, int>, struct RegionParameters > > Meric::parameter;
RegionParameters Meric::ignoreSettings = {};
std::vector <std::string> Meric::ignoreRegions;
std::vector<std::string> Meric::_name;
std::vector<int> Meric::_stack;
RegionParameters Meric::defaultParameter;
SocketsRegionParameters Meric::currentState;
std::string Meric::_mainRegion;
timespec Meric::_startTime;
timespec Meric::_stopTime;


void Meric::readRegionParameters()
{
	if (getDefaultParameters())
		exit(1);

	if (! Meric::REGION_OPTIONS.empty())
		readRegionOptions();
}

int Meric::getDefaultParameters()
{
	int ret=0;
//OBLIGATORY SECTION
	defaultParameter.nthreads        = mutils::getenv("MERIC_NUM_THREADS", -1);
	defaultParameter.frequency       = mutils::getenv("MERIC_FREQUENCY", 0);
	defaultParameter.uncoreFrequency = mutils::getenv("MERIC_UNCORE_FREQUENCY", 0);

#if defined (HAVE_CPUFREQ_H) || HAVE_X86_ADAPT_H
	if (defaultParameter.frequency == 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_FREQUENCY\n";
		ret = 1;
	}
#endif

#ifdef HAVE_X86_ADAPT_H
	if (defaultParameter.uncoreFrequency == 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_UNCORE_FREQUENCY\n";
		ret = 1;
	}
#endif

#ifdef HAVE_OMP_H
	if (defaultParameter.nthreads < 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_NUM_THREADS\n"
				  << "MERIC INFO: if the application doesn't use openMP, export MERIC_NUM_THREADS=0\n";
		ret = 1;
	}
	if (mutils::getenv("OMP_NUM_THREADS", 0))
	{
		std::cerr << "MERIC WARNIG: do not use OMP_NUM_THREADS\n";
	}
#endif

	if (ret)
		return ret;

//OPTIONAL SECTION
	MERIC_INFO << "Default number of threads: " << defaultParameter.nthreads << "\n";
	MERIC_INFO << "Default frequency: " << defaultParameter.frequency << "\n";
	MERIC_INFO << "Default uncore frequency: " << defaultParameter.uncoreFrequency << "\n";

	if (Meric::MODE < MODE_HDEEM || Meric::MODE > MODE_JETSON)
	{
		std::cerr << "MERIC WARNING: MERIC MODE unknown value - default "<< MODE_HDEEM <<std::endl;
		Meric::MODE = MODE_HDEEM;
	}
	
	if (Meric::COUNTERS != "none")
	{
		if (Meric::COUNTERS != "papi" && Meric::COUNTERS != "perfevent")
		{
			std::cerr << "MERIC WARNING: unknown counter type "<<Meric::COUNTERS<<std::endl;
			Meric::COUNTERS = "none";
		}
	}
	
	return 0;
}

void Meric::readRegionOptions()
{
//load input file
	std::string line, jsonString = "";
	std::ifstream jsonFile (Meric::REGION_OPTIONS);

	if (jsonFile.is_open())
	{
		while (getline(jsonFile,line))
		{
			jsonString += " " + line;
		}
		jsonFile.close();
	}
	const char * json = jsonString.c_str();

// parse input json
	struct json_value_s* root = json_parse(json, strlen(json));
	if (root ==  NULL)
	{
		std::cerr << "MERIC ERROR: empty or invalid input json\n";
		return;
	}

	struct json_object_element_s* data = ((struct json_object_s*)root->payload)->start;
	if (data == NULL)
	{
		free(root);
		return;
	}

//	process the settings into a structure
	struct json_object_element_s* region;
	struct json_object_element_s* node;
	struct json_object_element_s* socket;
	
	do
	{
		if (data->name->string[0] != '@')	//region
		{
			parameter[std::string(data->name->string)][std::make_pair(-1, -1)] = getRegionSettings(data);
		}
		else if (std::string(data->name->string) == "@IGNORE")
		{
			MERIC_INFO << data->name->string <<"\n";
			struct json_object_element_s* ignore = ((struct json_object_s*)data->value->payload)->start;
			do
			{
				if (std::string(ignore->name->string) == "@REGIONS")
				{
					struct json_array_element_s* set = ((struct json_array_s*)ignore->value->payload)->start;
					do
					{
						MERIC_INFO << "\t" << ((struct json_string_s*)set->value->payload)->string << std::endl;
						ignoreRegions.push_back(std::string(((struct json_string_s*)set->value->payload)->string));
						set = set->next;
					}while(set != NULL);
				}
				else if (std::string(ignore->name->string) == "@CHANGE")
				{
					ignoreSettings = getRegionSettings(ignore);
				}
				ignore = ignore->next;
			}while(ignore != NULL);
		}
		else if(std::string(data->name->string) == "@NODE")
		{
			MERIC_INFO << data->name->string <<"\n";
			node = ((struct json_object_s*)data->value->payload)->start;

			do{
				//list of regions
				struct json_object_element_s* socketOrRegion = ((struct json_object_s*)node->value->payload)->start;
				do{
					if(std::string(socketOrRegion->name->string) == "@SOCKET")
					{
						socket = ((struct json_object_s*)socketOrRegion->value->payload)->start;
						do{
							//list of regions
							region = ((struct json_object_s*)socket->value->payload)->start;
							do{
								parameter[std::string(region->name->string)][std::make_pair(std::stoi(std::string(node->name->string)), std::stoi(std::string(socket->name->string)))] = getRegionSettings(region);
								region = region->next;
							}while (region != NULL);
							socket = socket->next;
						}while (socket != NULL);
					}
					else	//list of regions
					{
						parameter[std::string(socketOrRegion->name->string)][std::make_pair(std::stoi(std::string(node->name->string)), -1)] = getRegionSettings(socketOrRegion);
					}
					socketOrRegion = socketOrRegion->next;
				}while (socketOrRegion != NULL);

				node = node->next;
			}while(node != NULL);

		}
		else if(std::string(data->name->string) == "@SOCKET")
		{
			std::cerr << data->name->string <<"\n";
			socket = ((struct json_object_s*)data->value->payload)->start;
			do{
				//list of regions
				region = ((struct json_object_s*)socket->value->payload)->start;
				do{
					parameter[std::string(region->name->string)][std::make_pair(-1, std::stoi(std::string(socket->name->string)))] = getRegionSettings(region);
					region = region->next;
				}while (region != NULL);
				socket = socket->next;
			}while(socket != NULL);
		}
		data = data->next;

	}while(data != NULL);

/*
//print the parameters (core frequency only)
	for ( const auto &regs : parameter )
	{
		for(const auto & pair : parameter[regs.first])	
		{
			MERIC_INFO << regs.first << "\n"
				<< "\t" << pair.first.first << ":" << pair.first.second<< "\n"
				<< "\t" << pair.second.frequency << "\n\n";
		}
	}
*/
	free(root);
}

RegionParameters Meric::getRegionSettings(json_object_element_s * data)
{
	RegionParameters param = {};
	struct json_object_element_s* set = ((struct json_object_s*)data->value->payload)->start;
	
	do
	{
		if ("FREQUENCY" == std::string(set->name->string))
				param.frequency = std::stoi(std::string(((struct json_string_s*)set->value->payload)->string));
		else if ("UNCORE_FREQUENCY" == std::string(set->name->string))
				param.uncoreFrequency = std::stoi(std::string(((struct json_string_s*)set->value->payload)->string));
		else if ("NUM_THREADS" == std::string(set->name->string))
				param.nthreads = std::stoi(std::string(((struct json_string_s*)set->value->payload)->string));
		else
				std::cerr << "MERIC ERROR: invalid region options key - " << set->name->string << std::endl;
		set = set->next;
	}while (set != NULL);
	
	return param;
}
