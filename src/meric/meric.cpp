
#include "meric.h"

using namespace meric;

int         Meric::MODE           = mutils::getenv<int>("MERIC_MODE", MODE_RUN);
bool        Meric::DETAILED       = mutils::getenv<bool>("MERIC_DETAILED", false);
bool        Meric::AGGREGATE      = mutils::getenv<bool>("MERIC_AGGREGATE", true);
bool        Meric::DEBUG          = mutils::getenv<bool>("MERIC_DEBUG", false);
bool        Meric::SAMPLES        = mutils::getenv<bool>("MERIC_SAMPLES", false);
bool        Meric::HDF5           = mutils::getenv<bool>("MERIC_HDF5", true);
std::string Meric::COUNTERS       = mutils::getenv<std::string>("MERIC_COUNTERS", "none");
std::string Meric::BARRIERS       = mutils::getenv<std::string>("MERIC_BARRIERS", "all");
std::string Meric::OUTPUT_DIR     = mutils::getenv<std::string>("MERIC_OUTPUT_DIR", "mericMeasurement");
std::string Meric::REGION_OPTIONS = mutils::getenv<std::string>("MERIC_REGION_OPTIONS", "");
std::string Meric::MEASURE        = mutils::getenv<std::string>("MERIC_MEASURE", "NONE");


std::map<std::string, std::map<std::pair<int, int>, struct RegionParameters > > Meric::parameter;
RegionParameters Meric::ignoreSettings = {};
std::vector <std::string> Meric::ignoreRegions;
std::vector<std::string> Meric::_name;
std::vector<int> Meric::_stack;
RegionParameters Meric::defaultParameter;
SocketsRegionParameters Meric::currentState;
RegionParameters Meric::outputFormat;
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
#if defined HAVE_CPUFREQ_H || defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_GEOPM_H
	defaultParameter.frequency = mutils::frequencyToHz(mutils::getenv("MERIC_FREQUENCY", std::string("")));
	if (defaultParameter.frequency < 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_FREQUENCY [Hz]\n";
		ret = 1;
	}
	else if (defaultParameter.frequency != 0)
	{
		Meric::outputFormat.frequency = int(true);
	}
#else
	defaultParameter.frequency = 0;
#endif

#if defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_GEOPM_H
	defaultParameter.uncoreFrequency = mutils::frequencyToHz(mutils::getenv("MERIC_UNCORE_FREQUENCY", std::string("")));
	if (defaultParameter.uncoreFrequency < 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_UNCORE_FREQUENCY [Hz]\n";
		ret = 1;
	}
	else if (defaultParameter.uncoreFrequency != 0)
	{
		Meric::outputFormat.uncoreFrequency = int(true);
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
		Meric::outputFormat.nthreads = int(true);
	}
#else
	defaultParameter.nthreads = 0;
#endif

#if defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_GEOPM_H
	defaultParameter.powerCapPKG     = mutils::getenv("MERIC_PWRCAP_POWER", -1);
	defaultParameter.powerCapPKGtime = mutils::getenv("MERIC_PWRCAP_TIME", -1);
	if (defaultParameter.powerCapPKG < 0 || defaultParameter.powerCapPKGtime < 0)
	{
		std::cerr << "MERIC ERROR: set MERIC_PWRCAP_POWER [mW] and MERIC_PWRCAP_TIME [ms]\n";
		ret = 1;
	}
	else if (defaultParameter.powerCapPKG != 0)
	{
		Meric::outputFormat.powerCapPKG = int(true);
		if (defaultParameter.powerCapPKGtime != 0)
		{
			Meric::outputFormat.powerCapPKGtime = int(true);
		}
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
		Meric::outputFormat.frequencyGPU = int(true);
	}
#else
	defaultParameter.frequencyGPU = 0;
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
		Meric::outputFormat.frequencyGPUmem = int(true);
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
		Meric::outputFormat.powerCapGPU = int(true);
	}
#else
	defaultParameter.powerCapGPU = 0;
#endif

#ifndef HAVE_HDF5_H
	Meric::HDF5 = false;
#endif 

	if (ret)
		return ret;

	size_t pos = 0;
	std::string OUTPUT_FORMAT  = mutils::getenv<std::string>("MERIC_OUTPUT_FORMAT", "");
	if (OUTPUT_FORMAT.size())
		OUTPUT_FORMAT+='_';
	while ((pos = OUTPUT_FORMAT.find('_')) != std::string::npos)
	{
		std::string token = OUTPUT_FORMAT.substr(0, pos);
		if (token == "CF")
			Meric::outputFormat.frequency = int(true);
		else if (token == "UnCF")
			Meric::outputFormat.uncoreFrequency = int(true);
		else if (token == "thrds")
			Meric::outputFormat.nthreads = int(true);
		else if (token == "CPUPWR")
			Meric::outputFormat.powerCapPKG = int(true);
		else if (token == "CPUPWRtw")
			Meric::outputFormat.powerCapPKGtime = int(true);
		else if (token == "GPUF")
			Meric::outputFormat.frequencyGPU = int(true);
		else if (token == "GPUMEMF")
			Meric::outputFormat.frequencyGPUmem = int(true);
		else if (token == "GPUPWR")
			Meric::outputFormat.powerCapGPU = int(true);
		else
		{
			std::cerr << "MERIC error: Invalid MERIC_OUTPUT_FORMAT : " << token << "\n";
			return 1;
		}
		OUTPUT_FORMAT.erase(0, pos + 1);
	}
	
	if (Meric::outputFormat.frequency)
		Meric::outputFilenameInfo +="CF_";
	if (Meric::outputFormat.uncoreFrequency)
		Meric::outputFilenameInfo +="UnCF_";
	if (Meric::outputFormat.nthreads)
		Meric::outputFilenameInfo +="thrds_";
	if (Meric::outputFormat.powerCapPKG)
		Meric::outputFilenameInfo +="CPUPWR_";
	if (Meric::outputFormat.powerCapPKGtime)
		Meric::outputFilenameInfo +="CPUPWRtw_";
	if (Meric::outputFormat.frequencyGPU)
		Meric::outputFilenameInfo +="GPUF_";
	if (Meric::outputFormat.frequencyGPUmem)
		Meric::outputFilenameInfo +="GPUMEMF_";
	if (Meric::outputFormat.powerCapGPU)
		Meric::outputFilenameInfo +="GPUPWR_";
	

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
			Meric::OUTPUT_FILENAME = "default";
			Meric::outputFilenameInfo = "DEF";
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
		
		if (modeString.count(token) == 0)
		{
			std::cerr << "MERIC ERROR: invalid input MEASUREMENT mode '"<<token<<"'\n";
			return 1;
		}
		else
		{
			MONITORS.push_back(modeString[token]);
			
			#ifndef HAVE_HDEEM_H
				if (modeString[token] == MODE_HDEEM_SAMPLES || modeString[token] == MODE_HDEEM_COUNTERS)
				{
					std::cerr << "MERIC WARNING: HDEEM power monitoring not available\n";
					continue;
				}
			#endif
			#if !defined(HAVE_MSR_SAFE) && !defined(HAVE_X86_ADAPT_H) && !defined(HAVE_GEOPM_H)
				if (modeString[token] == MODE_RAPL)
				{
					std::cerr << "MERIC WARNING: RAPL power monitoring not available\n";
					continue;
				}
			#endif
			#ifndef HAVE_NVML_H
				if (modeString[token] == MODE_NVML)
				{
					std::cerr << "MERIC WARNING: NVML power monitoring not available\n";
					continue;
				}
			#endif
			#ifndef HAVE_OCC
				if (modeString[token] == MODE_OCC)
				{
					std::cerr << "MERIC WARNING: OCC power monitoring not available\n";
					continue;
				}
			#endif
			#ifndef HAVE_RESTCLIENT_CPP_RESTCLIENT_H
				if (modeString[token] == MODE_DAVIDE)
				{
					std::cerr << "MERIC WARNING: DAVIDE power monitoring not available\n";
					continue;
				}
			#endif
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
			parameter[std::string(data->name->string)][std::make_pair(-1, -1)] = getRegionSettings(data, root);
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
					ignoreSettings = getRegionSettings(ignore, root);
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
								parameter[std::string(region->name->string)][std::make_pair(std::stoi(std::string(node->name->string)), std::stoi(std::string(socket->name->string)))] = getRegionSettings(region, root);
								region = region->next;
							}while (region != NULL);
							socket = socket->next;
						}while (socket != NULL);
					}
					else	//list of regions
					{
						parameter[std::string(socketOrRegion->name->string)][std::make_pair(std::stoi(std::string(node->name->string)), -1)] = getRegionSettings(socketOrRegion, root);
					}
					socketOrRegion = socketOrRegion->next;
				}while (socketOrRegion != NULL);

				node = node->next;
			}while(node != NULL);

		}
		else if(std::string(data->name->string) == "@SOCKET")
		{
			socket = ((struct json_object_s*)data->value->payload)->start;
			do{
				//list of regions
				region = ((struct json_object_s*)socket->value->payload)->start;
				do{
					parameter[std::string(region->name->string)][std::make_pair(-1, std::stoi(std::string(socket->name->string)))] = getRegionSettings(region, root);
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

RegionParameters Meric::getRegionSettings(json_object_element_s * data, json_value_s* root)
{
	if (data->value->type != json_type_object)
	{
		std::cerr << "MERIC ERROR: invalid input configuration file.\n";
		free(root);
		exit(1);
	}

	RegionParameters param = {};
	struct json_object_element_s* set = ((struct json_object_s*)data->value->payload)->start;

	do
	{
	std::cerr << std::string(set->name->string) << "\n";
		const std::string label = std::string(set->name->string);
		if ("FREQUENCY" == label)
		{
			if (defaultParameter.frequency != 0)
				param.frequency = mutils::frequencyToHz(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else if ("UNCORE_FREQUENCY" == label)
		{
			if (defaultParameter.uncoreFrequency != 0)
				param.uncoreFrequency = mutils::frequencyToHz(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else if ("NUM_THREADS" == label)
		{
			if (defaultParameter.nthreads != 0)
				param.nthreads = std::stoi(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else if ("PWRCAP_POWER" == label)
		{
			if (defaultParameter.powerCapPKG != 0)
				param.powerCapPKG = std::stol(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else if ("PWRCAP_TIME" == label)
		{
			if (defaultParameter.powerCapPKGtime != 0)
				param.powerCapPKGtime = std::stol(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else if ("GPU_FREQUENCY" == label)
		{
			if (defaultParameter.frequencyGPU != 0)
				param.frequencyGPU = mutils::frequencyToHz(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else if ("GPU_MEM_FREQUENCY" == label)
		{
			if (defaultParameter.frequencyGPUmem != 0)
				param.frequencyGPUmem = mutils::frequencyToHz(std::string(((struct json_string_s*)set->value->payload)->string));
		}
		else if ("PWRCAP_GPU" == label)
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
