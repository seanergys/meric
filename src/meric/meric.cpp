
#include "meric.h"

using namespace meric;

int         Meric::ITERATION  = mutils::getenv<int>("MERIC_ITERATION", 0);
int         Meric::MODE       = mutils::getenv<int>("MERIC_MODE" , MODE_RUN);
bool        Meric::CONTINUAL  = mutils::getenv("MERIC_CONTINUAL" , true);
bool        Meric::DETAILED   = mutils::getenv("MERIC_DETAILED"  , false);
bool        Meric::AGGREGATE  = mutils::getenv("MERIC_AGGREGATE" , true);
bool        Meric::DEBUG      = mutils::getenv("MERIC_DEBUG"     , false);
bool        Meric::SAMPLES    = mutils::getenv("MERIC_SAMPLES"   , false);
bool        Meric::HDF5       = mutils::getenv("MERIC_HDF5"      , true);
std::string Meric::COUNTERS   = mutils::getenv("MERIC_COUNTERS"  , std::string("none"));
std::string Meric::BARRIERS   = mutils::getenv("MERIC_BARRIERS"  , std::string("all"));
std::string Meric::OUTPUT_DIR = mutils::getenv("MERIC_OUTPUT_DIR", std::string("mericMeasurement"));
std::string Meric::REGION_OPTIONS  = mutils::getenv("MERIC_REGION_OPTIONS", std::string(""));
std::string Meric::MEASURE    = mutils::getenv<std::string>("MERIC_MEASURE", "NONE");

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
std::vector <mode> Meric::MONITORS;


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
#if defined HAVE_CPUFREQ_H || defined HAVE_X86_ADAPT_H || defined HAVE_MSR_CORE_H || defined HAVE_GEOPM_H
	defaultParameter.frequency = mutils::frequencyToHz(mutils::getenv("MERIC_FREQUENCY", std::string("")));
	if (defaultParameter.frequency < 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_FREQUENCY [Hz]\n";
		ret = 1;
	}
	else if (defaultParameter.frequency != 0)
	{
		Meric::outputFilenameInfo +="CF_";
	}
#else
	defaultParameter.frequency = 0;
#endif

#if defined HAVE_X86_ADAPT_H || defined HAVE_MSR_CORE_H || defined HAVE_GEOPM_H
	defaultParameter.uncoreFrequency = mutils::frequencyToHz(mutils::getenv("MERIC_UNCORE_FREQUENCY", std::string("")));
	if (defaultParameter.uncoreFrequency < 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_UNCORE_FREQUENCY [Hz]\n";
		ret = 1;
	}
	else if (defaultParameter.uncoreFrequency != 0)
	{
		Meric::outputFilenameInfo +="UnCF_";
	}
#else
	defaultParameter.uncoreFrequency = 0;
#endif

#ifdef HAVE_OMP_H
	defaultParameter.nthreads = mutils::getenv("MERIC_NUM_THREADS", -1);
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
#else
	defaultParameter.nthreads = 0;
#endif

#if defined HAVE_X86_ADAPT_H || defined HAVE_MSR_CORE_H || defined HAVE_GEOPM_H
	defaultParameter.powerCapPKG     = mutils::getenv("MERIC_PWRCAP_POWER", -1);
	defaultParameter.powerCapPKGtime = mutils::getenv("MERIC_PWRCAP_TIME", -1);
	if (defaultParameter.powerCapPKG < 0 || defaultParameter.powerCapPKGtime < 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_PWRCAP_POWER [mW] and MERIC_PWRCAP_TIME [ms]\n";
		ret = 1;
	}
	else if (defaultParameter.powerCapPKG != 0)
	{
		Meric::outputFilenameInfo +="CPUPWR_";
		if (defaultParameter.powerCapPKGtime != 0)
			Meric::outputFilenameInfo +="Twindow_";
	}
	else if (defaultParameter.powerCapPKGtime != 0)
	{
		std::cerr << "MERIC ERROR: MERIC_PWRCAP_POWER [mW] must be set to change MERIC_PWRCAP_TIME [ms]\n";
		ret = 1;
	}
#else
	defaultParameter.powerCapPKG = 0;
	defaultParameter.powerCapPKGtime = 0;
#endif

#ifdef HAVE_NVML_H
	defaultParameter.frequencyGPU = mutils::frequencyToHz(mutils::getenv("MERIC_GPU_FREQUENCY", std::string("")));
	if (defaultParameter.frequencyGPU < 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_GPU_FREQUENCY [Hz]\n";
		ret = 1;
	}
	else if (defaultParameter.frequencyGPU != 0)
	{
		Meric::outputFilenameInfo +="GPUF_";
	}
#else
	defaultParameter.frequencyGPU = 0
#endif

#ifdef HAVE_NVML_H
	defaultParameter.frequencyGPUmem = mutils::frequencyToHz(mutils::getenv("MERIC_GPU_MEM_FREQUENCY", std::string("")));
	if (defaultParameter.frequencyGPUmem < 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_GPU_MEM_FREQUENCY [Hz]\n";
		ret = 1;
	}
	else if (defaultParameter.frequencyGPUmem != 0)
	{
		Meric::outputFilenameInfo +="GPUMEMF_";
	}
#else
	defaultParameter.frequencyGPUmem = 0;
#endif

#ifdef HAVE_NVML_H
	defaultParameter.powerCapGPU = mutils::getenv("MERIC_PWRCAP_GPU", -1);
	if (defaultParameter.powerCapGPU < 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_PWRCAP_GPU [mW]\n";
		ret = 1;
	}
	else if (defaultParameter.powerCapGPU != 0)
	{
		Meric::outputFilenameInfo +="GPUPWR_";
	}
#else
	defaultParameter.powerCapGPU = 0;
#endif

#ifndef HAVE_HDF5_H
	Meric::HDF5 = false;
#endif 

	if (ret)
		return ret;

//OPTIONAL SECTION
	MERIC_INFO << "Default number of threads: " << defaultParameter.nthreads << "\n"
	           << "Default CPU frequency: " << defaultParameter.frequency << " Hz\n"
	           << "Default CPU uncore frequency: " << defaultParameter.uncoreFrequency << " Hz\n"
	           << "Default CPU powercap: " << defaultParameter.powerCapPKG << " mW\t" << defaultParameter.powerCapPKGtime << " ms\n"
	           << "Default GPU streaming multiprocessors frequency: " << defaultParameter.frequencyGPU << " Hz\n"
	           << "Default GPU memory frequency: " << defaultParameter.frequencyGPUmem << " Hz\n"
	           << "Default GPU powercap: " << defaultParameter.powerCapGPU << " mW\n";

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
	
	if (Meric::MODE != MODE_RUN)
	{
		std::cerr << "MERIC ERROR: MERIC_MODE option not supported any more, use MERIC_MEASURE instead\n";
		return 1;
	}

	while (Meric::MEASURE != "")
	{
		std::string token;
		size_t pos = Meric::MEASURE.find_first_of(',');
		if(pos != std::string::npos)
		{
			token = Meric::MEASURE.substr(0,pos);
			Meric::MEASURE = Meric::MEASURE.substr(pos+1, Meric::MEASURE.size()-pos);
		}
		else
		{
			token = Meric::MEASURE;
			Meric::MEASURE = "";
		}
		std::transform(token.begin(), token.end(), token.begin(), ::toupper);
		if (token == HDEEM::getSourceLabel())
		{
			MONITORS.push_back(MODE_HDEEM);
			#ifndef HAVE_HDEEM_H
				std::cerr << "MERIC WARNING: HDEEM power monitoring not available\n";
			#endif
		}
		else if (token == RAPL::getSourceLabel())
		{
			MONITORS.push_back(MODE_RAPL);
			#if !defined(HAVE_MSR_CORE_H) && !defined(HAVE_X86_ADAPT_H) && !defined(HAVE_GEOPM_H)
				std::cerr << "MERIC WARNING: RAPL power monitoring not available\n";
			#endif
		}
		else if (token == NVML::getSourceLabel())
		{
			MONITORS.push_back(MODE_NVML);
			#ifndef HAVE_NVML_H
				std::cerr << "MERIC WARNING: NVML power monitoring not available\n";
			#endif
		}
		else if (token == THUNDER::getSourceLabel())
			MONITORS.push_back(MODE_THUNDER);
		else if (token == JETSON::getSourceLabel())
			MONITORS.push_back(MODE_JETSON);
		else if (token == DAVIDE::getSourceLabel())
		{
			MONITORS.push_back(MODE_DAVIDE);
			#ifndef HAVE_RESTCLIENT_CPP_RESTCLIENT_H
				std::cerr << "MERIC WARNING: DAVIDE power monitoring not available\n";
			#endif
		}
		else if (token == "TIME")
			MONITORS.push_back(MODE_TIME);
		else if (token == "NONE")
		{
			MONITORS.clear();
			break;
		}
		else
		{
			std::cerr << "MERIC ERROR: invalid input MEASUREMENT mode '"<<token<<"'\n";
			return 1;
		}	
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
		if (Meric::COUNTERS != "papi" && Meric::COUNTERS != "perfevent" && Meric::COUNTERS != "msr")
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
		else if ("GPU_FREQUENCY" == std::string(set->name->string))
		{
			if (defaultParameter.frequencyGPU != 0)
				param.frequencyGPU = mutils::frequencyToHz(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else if ("GPU_MEM_FREQUENCY" == std::string(set->name->string))
		{
			if (defaultParameter.frequencyGPUmem != 0)
				param.frequencyGPUmem = mutils::frequencyToHz(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else if ("PWRCAP_GPU" == std::string(set->name->string))
		{
			if (defaultParameter.powerCapGPU != 0)
				param.powerCapGPU = std::stol(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else
			std::cerr << "MERIC ERROR: invalid region options key - " << set->name->string << std::endl;
		set = set->next;
	}while (set != NULL);
	
	return param;
}
