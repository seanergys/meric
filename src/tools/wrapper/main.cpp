#include <fstream>

#include "../../basis/json.h"
#include "algorithms/exhaustive.hpp"
#include "algorithms/randomtest.hpp"
#include "algorithms/postanalysis.hpp"


/**
 * Generates a json file, input configuration file of teh MERICwrapper
 * TODO prepsat na generovani souborou automaticky ze struktury cfg
 **/
void generateDefaultWrapperConfigFile()
{
//	configurationStateSpace CSS;
//	CSS.constructStateSpace();
	meric::RegionParameters defaultParameter;
	meric::Environment::init(meric::SYS_OTHER, defaultParameter, 0);
	meric::systemLimits limit = meric::Environment::getSystemSettings();
	std::vector <int> onlineThreads;
	int SOCKETS_PER_NODE, CORES_PER_SOCKET, THREADS_PER_CORE;
	meric::Environment::initHWSizes(SOCKETS_PER_NODE, CORES_PER_SOCKET, THREADS_PER_CORE, onlineThreads);
	
	std::ifstream frequencyBounderyFile;
	std::string freqBound = "";
	frequencyBounderyFile.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies");
	if (frequencyBounderyFile.is_open())
	{
		getline(frequencyBounderyFile, freqBound);
		frequencyBounderyFile.close();
		while(freqBound.back() == ' ')
			freqBound.pop_back();
		//std::replace(freqBound.begin(),freqBound.end(), ' ', ',');
		size_t pos= std::string::npos;  //freqBound.find('.');
		while ((pos = freqBound.find(' ')) != std::string::npos)
			freqBound.replace(pos, 1, "000,");
		freqBound += "000";
	}

	std::string content = "{\n";
	content +=std::string("\t\"MPI\" : \"true\",\n")
	        + "\t\"BASELINE\" : 0,\n";

	content+=std::string("\t\"PARAMETERS\" : {\n")
	       + "\t\t\"FREQUENCY\" : {\n"
	       + "\t\t\t\"MAX\" : "+std::to_string(limit.maxCoreFreq)+",\n"
	       + "\t\t\t\"MIN\" : "+std::to_string(limit.minCoreFreq)+",\n";
	if (freqBound == "")
		content+= "\t\t\t\"STEP\":  100000000\n";
	else
		content+= "\t\t\t\"LIST\" : [\n" + freqBound +"\n\t\t\t]\n";
	content+="\t\t},\n";
	if (limit.maxUncoreFreq > 0 && limit.minUncoreFreq > 0)
		content+=std::string("\t\t\"UNCORE_FREQUENCY\" : {\n")
				+ "\t\t\t\"MAX\" : "+std::to_string(limit.maxUncoreFreq)+",\n"
				+ "\t\t\t\"MIN\" : "+std::to_string(limit.minUncoreFreq)+",\n"
				+ "\t\t\t\"STEP\":  100000000\n"
				+ "\t\t},\n";
	content+=std::string("\t\t\"NUM_THREADS\" : {\n")
			+ "\t\t\t\"MAX\" : "+std::to_string(onlineThreads.size())+",\n"
			+ "\t\t\t\"MIN\" :  1,\n"
			+ "\t\t\t\"STEP\":  1\n"
			+ "\t\t}\n"
			+ "\t},\n"; //end PARAMETERS


	content+= std::string("\t\"ALGORITHM\" : {\n")
			+ "\t\t\"NAME\" : \"EVO\",\n"
			+ "\t\t\"PARAMETERS\" : {\n"
			+ "\t\t\t\"END_CONDITION\" : 2,\n"
			+ "\t\t\t\"POPULATION\" : 10\n"
			+ "\t\t}\n"
			+ "\t},\n"
			+ "\t\"ALGORITHM\" : {\n"
			+ "\t\t\"NAME\" : \"EXHAUSTIVE\"\n"
			+ "\t},\n"
			+ "\t\"ALGORITHM\" : {\n"
			+ "\t\t\"NAME\" : \"PSO\",\n"
			+ "\t\t\"PARAMETERS\" : {\n"
			+ "\t\t\t\"END_CONDITION\" : 2,\n"
			+ "\t\t\t\"PARTICLES\" : 10,\n"
			+ "\t\t\t\"CONST_CP\"  : 2.05,\n"
			+ "\t\t\t\"CONST_CG\"  : 2.05,\n"
			+ "\t\t\t\"CONST_W\"   : 0.9\n"
			+ "\t\t}\n"
			+ "\t},\n";

	content+= std::string("\t\"MERIC\" : {\n")
			+ "\t\t\"MEASURE\"    : ";
	std::string measurementSource = "";
//----------------------------------------------------------------------------//
//--- TODO only one MEASUREMENT MODE parameter is currently supported --------//
/*
#ifdef HAVE_HDEEM_H
	measurementSource += "HDEEM-S,"
#endif
#if defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_GEOPM_H
	measurementSource += "RAPL,"
#endif
#ifdef HAVE_NVML_H
	measurementSource += "NVML,"
#endif
#ifdef HAVE_OCC
	measurementSource += "OCC,"
#endif
#ifdef HAVE_RESTCLIENT_CPP_RESTCLIENT_H
	measurementSource += "DAVIDE,"
#endif
*/
#ifdef HAVE_HDEEM_H
	measurementSource += "HDEEM-S,"
#elif defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_GEOPM_H
	measurementSource += "RAPL,"
#elif defined HAVE_NVML_H
	measurementSource += "NVML,"
#elif defined HAVE_OCC
	measurementSource += "OCC,"
#elif defined HAVE_RESTCLIENT_CPP_RESTCLIENT_H
	measurementSource += "DAVIDE,"
#endif
//----------------------------------------------------------------------------//
	if (measurementSource == "") //just time measurement
	{
		measurementSource = "\"TIME\",\n";
	}
	else
	{
		measurementSource.pop_back();
		measurementSource = "\""+ measurementSource +"\",\n";
	}
	content += measurementSource
#if defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_GEOPM_H
			+ "\t\t\"PWRCAP_POWER\" : 0,\n"
			+ "\t\t\"PWRCAP_TIME\"  : 0,\n"
#endif
#ifdef HAVE_MSR_SAFE
			+ "\t\t\"COUNTERS\" : \"msr\",\n"
#elif defined HAVE_PAPI_H
			+ "\t\t\"COUNTERS\" : \"papi\",\n"
#elif defined HAVE_PERFEVENT_H
			+ "\t\t\"COUNTERS\" : \"perfevent\",\n"
#endif
			+ "\t\t\"AGGREGATE\"  : 1,\n"
			+ "\t\t\"DETAILED\"   : 1,\n"
			+ "\t\t\"SAMPLES\"    : 0,\n"
			+ "\t\t\"BARRIERS\"   : \"all\",\n"
//			+ "\t\t\"OUTPUT_FILENAME\" : \"\",\n"
			+ "\t\t\"OUTPUT_DIR\" : \"mericMeasurement\"\n"
			+ "\t}\n";

	content+= "}\n";

	//CSS.StateSpaceClose();
	meric::Environment::close();

	std::ofstream configFile ("defaultWrapperConfig.json");
	configFile << content;
	configFile.close();
	std::cerr << "MERICwrapper - default config file generated: defaultWrapperConfig.json\n";
}


void runApplication(std::string applicationCommand, std::string outputDir, appConfiguration configuration)
{
	std::string mericConfig = "";
	std::string mericConfigFileName = outputDir+"/MERICwrapperTestconfiguration.json";
	if (configuration.size() != 1 || configuration[0].regionName != "") //export default static configuration
	{
		setenv("MERIC_FREQUENCY", "0", 1);
		setenv("MERIC_UNCORE_FREQUENCY", "0", 1);
		setenv("MERIC_NUM_THREADS", "0", 1);
		setenv("MERIC_PWRCAP_POWER", "0", 1);
		setenv("MERIC_PWRCAP_TIME", "0", 1);
		setenv("MERIC_GPU_FREQUENCY", "0", 1);
		setenv("MERIC_GPU_MEM_FREQUENCY", "0", 1);
		setenv("MERIC_PWRCAP_GPU", "0", 1); 
	}
	
	storeMERICconfig(configuration, mericConfigFileName);
	
	if (system(applicationCommand.c_str()) != 0)
	{
		std::cerr << "MERICwrapper ERROR: could not run `" << applicationCommand << "`\n";
		exit(1);
	}
	
	if (mericConfig != "")
		remove(mericConfigFileName.c_str());
}

void printHelp()
{
	std::cerr << std::endl << "MERICwrapper" << std::endl
	          << "----------------------------------------"
	          << "----------------------------------------" << std::endl
	          << "./wrapper -s config.json -- mpirun -np 2 ./a.out -a -x" << std::endl
	          << "----------------------------------------"
	          << "----------------------------------------" << std::endl
	          << "\t-d = generate default config file 'defaultWrapperConfig.json'." << std::endl
	          << "\t-s configFile = run application analysis according the configuration file." << std::endl
	          << "\t-a pathToDir = generate optimal configuration file from a data measured by MERIC." << std::endl
	          << "\t\t-m MERIC_MEASURE = measurement system that was used to measure the data, only one can be specified, no default." << std::endl
	          << "\t-b baseline = power baseline [W] that should be used for analysis, default 0 W." << std::endl
	          << "\t-g margin = energy consumption margin [%] of selected configuration, default 0 %. Select a configuration with energy consumption within the margin of the optimal configuration, but with better runtime." << std::endl
	          << "\t-e extention = maximum runtime extention [%], default 10 %." << std::endl
	          << "\t-- = follows command to run the application to analyze." << std::endl;
	exit(1);
}

void readParameters(int argc, char **argv, cfg & config)
{
/*
# TODO - reading parameters
# - number of iterations under one configuration
# - step size for each tuned parameter
# - maximum influence of the configuration to runtime
*/

	struct stat info;
	
	//reading parameters from command line
	if (argc == 1)
		printHelp();
	std::string settingsFile = "";
	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg == "-d") //default config file
		{
			generateDefaultWrapperConfigFile();
			exit(0);
		}
		else if (arg == "-a") //analysis of already measured data
		{
			config.meric.outputdir.value = std::string(argv[++i]);
			config.algorithm.name.value = "NONE";
		}
		else if (arg == "-b") //baseline
		{
			config.baseline.value = std::stod(argv[++i]);
		}
		else if (arg == "-g") //margin
		{
			config.margin.setValue(std::string(argv[++i]));
		}
		else if (arg == "-e") //extention	//TODO I need to load default run data first
		{
			config.extention.setValue(std::string(argv[++i]));
		}
		else if (arg == "-m") // MERIC_MEASURE
		{
			config.meric.measure.setValue(std::string(argv[++i]));
		}
		else if (arg == "-s") //start a new analysis, path to configuration file
		{
			settingsFile = std::string(argv[++i]);
			config.algorithm.name.value = config.algorithm.name.supportedValues.back();
		}
		else if (arg == "--")
		{
			i++;
			for (; i < argc; i++)
				config.application += " " + std::string(argv[i]);
		}
		else
			printHelp();
	}

 
	if (config.meric.outputdir.value != "") // analysis of already measured data
	{
		if (config.meric.measure.value == meric::MODE_TIME) //"TIME")
		{
			std::cerr << "ERROR: wrong input MERIC MEASUREMENT MODE (-m)\n";
			printHelp();
		}
		
/*	blbost - default.csv je adresarich jednotlivych regionu

		if(stat((config.meric.outputdir.value+"/default.csv").c_str(), &info) != 0) // TODO the other default file name
		{
			std::cerr << "ERROR: The data to analyse does not contain default run\n";
			printHelp();
		}
*/
		return;
	}
	else
	{
		struct timespec timeNow;
		clock_gettime(CLOCK_REALTIME, &timeNow);
		config.meric.outputdir.value = "MERICwrapper-"+std::to_string(timeNow.tv_sec); //default name
	}


// reading configuration file ------------------------------------------------//
	if (settingsFile == "" || config.application == "")
	{
		std::cerr << "ERROR: wrong input parameters\n";
		printHelp();
	}
	
	if(stat(settingsFile.c_str(), &info) != 0)
	{
		std::cerr << "ERROR: MERICwrapper configuration file "<< settingsFile <<" cannot be open\n";
		printHelp();
	}

	std::ifstream jsonFile (settingsFile);
	std::string line, jsonString = "";
	if (jsonFile.is_open())
	{
		while (getline(jsonFile,line))
		{
			jsonString += " " + line;
		}
		jsonFile.close();
	}
	const char * json = jsonString.c_str();
	struct json_value_s* root = json_parse(json, strlen(json));
	if (root ==  NULL)
	{
		std::cerr << "ERROR: invalid json file " << settingsFile << std::endl;
		printHelp();
	}
	loadConfigFile(((struct json_object_s*)root->payload)->start, &config);
	
	for (auto c : config.parameters.config)
	{
		if(CFGtunedParameter* lst = dynamic_cast<CFGtunedParameter*>(c.second))
		{
			lst->fillList();
		}
	}
	
	setenv("MERIC_OUTPUT_FORMAT", config.getOutputFormat().c_str(), 1);
	setenv("MERIC_MEASURE", config.meric.measure.getValue().c_str(), 1);
	setenv("MERIC_AGGREGATE", std::to_string(config.meric.aggregate.value).c_str(), 1);
	setenv("MERIC_DETAILED", std::to_string(config.meric.detailed.value).c_str(), 1);
	setenv("MERIC_SAMPLES", std::to_string(config.meric.samples.value).c_str(), 1);
	setenv("MERIC_BARRIERS", config.meric.barriers.value.c_str(), 1);
	setenv("MERIC_COUNTERS", config.meric.counters.value.c_str(), 1);

	setenv("MERIC_OUTPUT_DIR", config.meric.outputdir.value.c_str(), 1);
//#if __cplusplus >= 201703L
//	std::filesystem::remove_all(config.meric.outputdir.value);
//#else
//	if (system(("rm -rf "+config.meric.outputdir.value).c_str()) != 0)
//		std::cerr << "MERICwrapper WARNING: could not remove old " << config.meric.outputdir.value << std::endl;
//#endif

/*
	if (params.alg == EVO || params.alg == PSO)
	{
		if (availConfigs[0].size())
			params.iterations *= availConfigs[0].size();
		if (availConfigs[1].size())
			params.iterations *= availConfigs[1].size();
		if (availConfigs[2].size())
			params.iterations *= availConfigs[2].size();
		params.iterations /= POPULATION_SIZE;
	}
*/
}


////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	cfg config;
	readParameters(argc, argv, config);

//	exhaustiveSearch CSS;
//	postanalysis CSS;

	configurationStateSpace * CSS;
	if (config.algorithm.name.value == "EXHAUSTIVE")
	{
		std::cout << "exhaustive\n";
		CSS = new exhaustiveSearch();
		appConfiguration emptyConfiguration;
		runApplication(config.application, config.meric.outputdir.value, emptyConfiguration); //default run
	}
	else if (config.algorithm.name.value == "RANDOM")
	{
		std::cout << "randomtest\n";
		CSS = new randomTestSearch();
		appConfiguration emptyConfiguration;
		runApplication(config.application, config.meric.outputdir.value, emptyConfiguration);
	}
	//else if (config.algorithm.name.value == "EVO")
	//else if (config.algorithm.name.value == "PSO")
	else if (config.algorithm.name.value == "NONE")
	{
		std::cout << "post-analysis\n";
		CSS = new postanalysis();
	}
	else
	{
		std::cerr << "input file error\n";
		return 100;
	}
	
	// init analysis
	DataAnalysis::initDataAnalysis(config);
	if (DataAnalysis::regions.size() == 0 || DataAnalysis::dataFormat.size() == 0)
	{
		delete CSS;
		return 101;
	}
	CSS->constructStateSpace(config);

	//execute
	size_t newconfigurations = 0;
	std::vector<appConfiguration> testConfigurationsToTry;
	std::vector<appConfiguration> testConfigurationsDone;
	while (CSS->generateConfigurations(testConfigurationsToTry, testConfigurationsDone))
	{
		std::cout << "testConfigurationsToTry = " << testConfigurationsToTry.size() << "\n";
		while(testConfigurationsToTry.size())
		{
			CSS->cleanEnvironment();
			runApplication(config.application, config.meric.outputdir.value, testConfigurationsToTry[0]);
			testConfigurationsDone.push_back(testConfigurationsToTry[0]);
			testConfigurationsToTry.erase(testConfigurationsToTry.begin());
			newconfigurations++;
		}
	}
	
	std::cout << " * Optimal configuration stored into optimalConfig.opts * \n";


/*	
	std::vector<appConfiguration> space;
	CSS.generateConfigurations(space);
	for (appConfiguration run : space)
		runApplication(run);
*/
////////////////////////////////////////////////////////////////////////////////


/*
	std::vector<long> valueList = {10,15,20};
	coreFrequency * CFparam;
	CFparam = CFparam->getInstance(valueList, 20);
	
	CFparam->printList();

//	return 0;

	CFparam = CFparam->getInstance();
	CFparam->printList();
	
	std::cout << "ParVal 1: "<< typeid(CFparam).name() << '\n';
	parameterInterface<__typeof__(CFparam)> i;
	coreFrequency * CFparam3;
	CFparam3 = i.getInstance();
	CFparam3->printList();
	std::cout << "OK\n";
*/
//	coreFrequency * coreFrequency::getInstance(valueList, 20);
	
	
	
	//parameterValue<coreFrequency<long>> pv(200);
//	parameterValue<coreFrequency> pv(10);
/*	parameterValue<coreFrequency> pv1(300);
	parameterValue<coreFrequency> pv2(200);
	
	if (pv == pv1)
		std::cout << "same 1\n";
	
	if (pv == pv2)
		std::cout << "same 2\n";
*/


//	CSS.parameters.push_back(CFparam);
	
//	int i = 10;
//	int * j = &i;
//	CSS.tmp.push_back(j);
	
//	for(size_t x : CSS.getDimensionsSize())
//		std::cout << CSS.parameters[x]
	
	
//	regionConfiguration c("main");
//	c.addParameterValue(std::bind(&parameterValue<coreFrequency>::setParameter, &pv));


	delete CSS;
	
	
	return 0;

}
