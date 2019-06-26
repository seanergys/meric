
#include "meric.h"

using namespace meric;

int         Meric::ITERATION  = mutils::getenv("MERIC_ITERATION", 0);
int         Meric::MODE       = mutils::getenv<int>("MERIC_MODE" , MODE_RUN);
bool        Meric::CONTINUAL  = mutils::getenv("MERIC_CONTINUAL" , false);
bool        Meric::DETAILED   = mutils::getenv("MERIC_DETAILED"  , false);
bool        Meric::AGGREGATE  = mutils::getenv("MERIC_AGGREGATE" , true);
bool        Meric::DEBUG      = mutils::getenv("MERIC_DEBUG"     , false);
bool        Meric::SAMPLES    = mutils::getenv("MERIC_SAMPLES"   , false);
bool        Meric::HDF5       = mutils::getenv("MERIC_HDF5"      , true);
std::string Meric::COUNTERS   = mutils::getenv("MERIC_COUNTERS"  , std::string("none"));
std::string Meric::BARRIERS   = mutils::getenv("MERIC_BARRIERS"  , std::string("all"));
std::string Meric::OUTPUT_DIR = mutils::getenv("MERIC_OUTPUT_DIR", std::string("mericMeasurement"));
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
bool Meric::IGNORE = false;
bool Meric::MPIbarriers = true;
bool Meric::OMPbarriers = true;
std::string Meric::outputFilenameInfo = "";
std::string Meric::OUTPUT_FILENAME = "";


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
	defaultParameter.frequency       = mutils::frequencyToHz(mutils::getenv("MERIC_FREQUENCY", std::string("")));
	defaultParameter.uncoreFrequency = mutils::frequencyToHz(mutils::getenv("MERIC_UNCORE_FREQUENCY", std::string("")));
	defaultParameter.powerCapPKG     = mutils::getenv("MERIC_PWRCAP_POWER", -1);
	defaultParameter.powerCapPKGtime = mutils::getenv("MERIC_PWRCAP_TIME", -1);

#if defined (HAVE_CPUFREQ_H) || defined (HAVE_X86_ADAPT_H) || defined (HAVE_MSR_CORE_H)
	if (defaultParameter.frequency < 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_FREQUENCY [Hz]\n";
		ret = 1;
	}
	else if (defaultParameter.frequency != 0)
	{
		Meric::outputFilenameInfo +="CF_";
	}
	
#endif

#if defined (HAVE_X86_ADAPT_H) || defined (HAVE_MSR_CORE_H)
	if (defaultParameter.uncoreFrequency < 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_UNCORE_FREQUENCY [Hz]\n";
		ret = 1;
	}
	else if (defaultParameter.uncoreFrequency != 0)
	{
		Meric::outputFilenameInfo +="UnCF_";
	}
#endif

#ifdef HAVE_OMP_H
	if (defaultParameter.nthreads < 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_NUM_THREADS\n"
				  << "MERIC INFO: if the application doesn't use openMP, export MERIC_NUM_THREADS=0\n";
		ret = 1;
	}
	else if (defaultParameter.nthreads != 0)
	{
		Meric::outputFilenameInfo +="thrds_";
	}
#endif

#if defined (HAVE_X86_ADAPT_H) || defined (HAVE_MSR_CORE_H)
	if (defaultParameter.powerCapPKG < 0 || defaultParameter.powerCapPKGtime < 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_PWRCAP_POWER [W] and MERIC_PWRCAP_TIME [us]\n";
		ret = 1;
	}
	else if (defaultParameter.powerCapPKG != 0)
	{
		Meric::outputFilenameInfo +="PWR_";
		if (defaultParameter.powerCapPKGtime != 0)
			Meric::outputFilenameInfo +="Twindow_";
	}
	else if (defaultParameter.powerCapPKGtime != 0)
	{
		std::cerr << "MERIC ERROR: MERIC_PWRCAP_POWER [W] must be set to change MERIC_PWRCAP_TIME [us]\n";
		ret = 1;
	}
#endif

#ifndef HAVE_HDF5_H
	Meric::HDF5 = false;
#endif 

	if (ret)
		return ret;

//OPTIONAL SECTION
	MERIC_INFO << "Default number of threads: " << defaultParameter.nthreads << "\n";
	MERIC_INFO << "Default frequency: " << defaultParameter.frequency << " Hz\n";
	MERIC_INFO << "Default uncore frequency: " << defaultParameter.uncoreFrequency << " Hz\n";
	MERIC_INFO << "Default powercap: " << defaultParameter.powerCapPKG << " W\t" << defaultParameter.powerCapPKGtime << " us\n";

	std::string outputFilenameSuffix = mutils::getenv("MERIC_OUTPUT_FILENAME", std::string(""));
	if (outputFilenameSuffix.size() == 0)
	{
		if (Meric::outputFilenameInfo.size() == 0)
		{
			Meric::OUTPUT_FILENAME = "log";
			Meric::outputFilenameInfo = "CFG";
		}
		else
		{
			Meric::outputFilenameInfo.pop_back();
		}
	}
	else
	{
		for (size_t i = 0; i < std::count(outputFilenameSuffix.begin(), outputFilenameSuffix.end(), '_')+1; i++)
		{
			Meric::outputFilenameInfo +="APP"+std::to_string(i)+"_";
		}
		if (Meric::outputFilenameInfo.size() > 0) //remove the last underscore
		{
			Meric::outputFilenameInfo.pop_back();
		}
		Meric::OUTPUT_FILENAME += outputFilenameSuffix;
	}
	
	if (Meric::MODE < MODE_HDEEM || Meric::MODE >= MODE_LIMIT)
	{
		std::cerr << "MERIC WARNING: MERIC MODE unknown value <"<<MODE_HDEEM<<","<<MODE_LIMIT-1<<"> - default "<< MODE_RUN <<std::endl;
		Meric::MODE = MODE_RUN;
	}
	
	if (Meric::BARRIERS != "all")
	{
		if (Meric::BARRIERS == "mpi")
		{
			Meric::OMPbarriers = false;
			std::cerr << "MERIC WARNING: OpenMP BARRIERS turned off\n";
		}
		else if (Meric::BARRIERS == "omp")
		{
			Meric::MPIbarriers = false;
			std::cerr << "MERIC WARNING: MPI BARRIERS turned off\n";
		}
		else if (Meric::BARRIERS == "none")
		{
			Meric::OMPbarriers = false;
			Meric::MPIbarriers = false;
			std::cerr << "MERIC WARNING: BARRIERS turned off\n";
		}
	}
	
	if (Meric::COUNTERS != "none")
	{
		if (Meric::COUNTERS != "papi" && Meric::COUNTERS != "perfevent")
		{
			if (Meric::COUNTERS != "")
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
		{
			if (defaultParameter.frequency != 0)
				param.frequency = mutils::frequencyToHz(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else if ("UNCORE_FREQUENCY" == std::string(set->name->string))
		{
			if (defaultParameter.uncoreFrequency != 0)
				param.uncoreFrequency = mutils::frequencyToHz(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else if ("NUM_THREADS" == std::string(set->name->string))
		{
			if (defaultParameter.nthreads != 0)
				param.nthreads = std::stoi(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else if ("PWRCAP_POWER" == std::string(set->name->string))
		{
			if (defaultParameter.powerCapPKG != 0)
				param.powerCapPKG = std::stol(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else if ("PWRCAP_TIME" == std::string(set->name->string))
		{
			if (defaultParameter.powerCapPKGtime != 0)
				param.powerCapPKGtime = std::stol(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else
			std::cerr << "MERIC ERROR: invalid region options key - " << set->name->string << std::endl;
		set = set->next;
	}while (set != NULL);
	
	return param;
}
