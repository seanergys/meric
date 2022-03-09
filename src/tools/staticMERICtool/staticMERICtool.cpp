
#include "../../wrapper/environmentwrapper.h"
#include "../../wrapper/raplwrapper.h"
#include "../../wrapper/nvmlwrapper.h"
#include <limits>

using namespace meric;

std::string OUTPUT_FILE;
int POWER_BASELINE = 0;

#ifdef START
template <typename Tmode> void storeValues(std::map<std::string, unsigned long long int> &record, double time);
#endif

#ifdef STOP
double readStoredValues(std::map<std::string, unsigned long long int> & oldRecord);
template <typename Tmode> void printValues(std::map<std::string, unsigned long long int> &oldRecord, std::map<std::string, unsigned long long int> &newRecord, double runtime, bool verbose);
#endif

//----------------------------------------------------------------------------//
#ifdef START
template <typename Tmode>
void storeValues(std::map<std::string, unsigned long long int> &record, double time)
{
	std::remove(OUTPUT_FILE.c_str());
	std::ofstream ss (OUTPUT_FILE, std::ofstream::out);
	//store time
	ss << std::setprecision(std::numeric_limits<double>::digits10 + 1) << time << std::endl;
	//store counters
	for(std::map<std::string, unsigned long long int>::iterator iterator = record.begin(); iterator != record.end(); iterator++)
	{
		if (Tmode::getSummaryCounterName() == iterator->first)
			continue;	
		ss << iterator->first << '\t' << iterator->second << std::endl;
	}
	ss.close();
}
#endif

#ifdef STOP
double readStoredValues(std::map<std::string, unsigned long long int> & oldRecord)
{
	std::ifstream ss (OUTPUT_FILE);
	if (!ss)
	{
		std::cerr << "ERROR: running energyMeasureStop before energyMeasureStart!\n";
		exit(5);
	}

	std::string counter;
	unsigned long long int value;
	double timeOld;
	ss >> timeOld;
	
	while (ss >> counter >> value)
	{
		oldRecord[counter] = value;
	}
	ss.close();
	std::remove(OUTPUT_FILE.c_str());
	return timeOld;
}

template <typename Tmode>
void printValues(std::map<std::string, unsigned long long int> &oldRecord, std::map<std::string, unsigned long long int> &newRecord, double runtime, bool verbose)
{
	std::map<std::string, unsigned long long int> record;
	double summary = 0.0;
	
	if (verbose)
	{
		std::cout << "Runtime [s] = " << runtime << std::endl;
		for(std::map<std::string, unsigned long long int>::iterator iterator = oldRecord.begin(); iterator != oldRecord.end(); iterator++)
		{
			double value = Tmode::getResultValue(oldRecord[iterator->first], newRecord[iterator->first], runtime, iterator->first);
			std::cout << Tmode::getCounterLabel(iterator->first) << " = " << value << std::endl;
			summary += value;
		}
		if (Tmode::getSummaryCounterName() != "")
		{
			summary += POWER_BASELINE*runtime;
			std::cout << std::endl << Tmode::getSummaryCounterName() << " = " << summary << std::endl;
		}
	}else
	{
		//TODO what if there is no summary
		for(std::map<std::string, unsigned long long int>::iterator iterator = oldRecord.begin(); iterator != oldRecord.end(); iterator++)
		{
			summary += Tmode::getResultValue(oldRecord[iterator->first], newRecord[iterator->first], runtime, iterator->first);
		}
		std::cout <<  summary + POWER_BASELINE*runtime << std::endl;
	}
}
#endif

//----------------------------------------------------------------------------//
//----------------------------------------------------------------- MAIN -----//
//----------------------------------------------------------------------------//
template <typename Tmode> 
void measureEnergy(bool verbose)
{
	Tmode::init(true);

	struct timespec timeNow;
	clock_gettime(CLOCK_REALTIME, &timeNow);

	std::map<std::string, unsigned long long int> recordNow;
	Tmode::getCounterValues(recordNow);

#ifdef START
	storeValues<Tmode>(recordNow, mutils::timeToDouble(timeNow));
#endif

#ifdef STOP
	std::map<std::string, unsigned long long int> recordOld;
	double timeOld = readStoredValues(recordOld);
	printValues<Tmode>(recordOld, recordNow, mutils::timeToDouble(timeNow)-timeOld, verbose);
	Environment::restoreSystemSettings(true);
#endif
}

void printHelp()
{
	std::cerr << "\nMERIC static tool help:\n\n"
	          << "Run energyMeasureStart to start the energy measurement and change system parameters\n"
	          << "Run energyMeasureStop to stop energy measurement, restore system settings and print the result energy consumption to std::cout\n"
	          << "\nenergy measurement mode (-e) is the only mandatory parameter\n\n"
	          << "\nenergyMeasureStart parameters:\n"
	          << "\t-e = energy measurement system \"RAPL\" or \"NVML\"\n"
	          << "\t-c = CPU core frequency [Hz]\n" //, specify value with units [GHz, MHz, kHz or default Hz]\n"
	          << "\t-u = CPU uncore frequency [Hz]\n" //, specify value with units [GHz, MHz, kHz or default Hz]\n"
	          << "\t-t = #OpenMP threads\n"
	          << "\t-p = CPU power capping power limit [mW]\n"
	          << "\t-w = CPU power capping time window [ms]\n"
	          << "\t-s = GPU SM frequency [Hz]\n"
	          << "\t-r = GPU memory frequency [Hz]\n"
	          << "\t-g = GPU power capping power limit [mW]\n"
	          << "\nenergyMeasureStop parameters:\n"
	          << "\t-e = energy measurement system \"RAPL\" or \"NVML\"\n"
	          << "\t-b = node baseline (static) power consumption [W]\n"
	          << "\t-q = print the overall consumed energy only [J]\n"
	          << std::endl;
	exit(1);
}

int main(int argc, char **argv)
{
	bool verbose = true; //used only if the STOP is defined
	char hostname[HOST_NAME_MAX];
	std::string energyMode = "";
	if (gethostname(hostname, HOST_NAME_MAX) != 0)
	{
		std::cerr << "missing machine hostname\n";
		exit(1);
	}
	OUTPUT_FILE = std::string(MERIC_PATH)+"/build/.MERICmeasure-" + std::string(hostname) +".data";
	RegionParameters defaultParameter;

	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
#ifdef START
		if (arg == "-c")
		{
			if (i + 1 < argc)
			{
				try
				{
					defaultParameter.frequency = std::stol(argv[++i]);
				} catch (std::exception const &e)
				{
					printHelp();
				}
			}else
			{
				printHelp();
			}
		}else if (arg == "-u")
		{
			if (i + 1 < argc)
			{
				try{
					defaultParameter.uncoreFrequency = std::stol (argv[++i]);
				} catch (std::exception const &e)
				{
					printHelp();
				}
			}else
			{
				printHelp();
			}
		}else if (arg == "-t")
		{
			if (i + 1 < argc)
			{
				try{
					defaultParameter.nthreads = std::stoi (argv[++i]);
				} catch (std::exception const &e)
				{
					printHelp();
				}
			}else
			{
				printHelp();
			}
		}else if (arg == "-p")
		{
			if (i + 1 < argc)
			{
				try{
					defaultParameter.powerCapPKG = std::stol (argv[++i]);
				} catch (std::exception const &e)
				{
					printHelp();
				}
			}else
			{
				printHelp();
			}
		}else if (arg == "-w")
		{
			if (i + 1 < argc)
			{
				try{
					defaultParameter.powerCapPKGtime = std::stol (argv[++i]);
				} catch (std::exception const &e)
				{
					printHelp();
				}
			}else
			{
				printHelp();
			}
		}else if (arg == "-s")
		{
			if (i + 1 < argc)
			{
				try{
					defaultParameter.frequencyGPU = std::stol (argv[++i]);
				} catch (std::exception const &e)
				{
					printHelp();
				}
			}else
			{
				printHelp();
			}
		}else if (arg == "-r")
		{
			if (i + 1 < argc)
			{
				try{
					defaultParameter.frequencyGPUmem = std::stol (argv[++i]);
				} catch (std::exception const &e)
				{
					printHelp();
				}
			}else
			{
				printHelp();
			}
		}else if (arg == "-g")
		{
			if (i + 1 < argc)
			{
				try{
					defaultParameter.powerCapGPU = std::stol (argv[++i]);
				} catch (std::exception const &e)
				{
					printHelp();
				}
			}else
			{
				printHelp();
			}
		}

#elif defined STOP

		if (arg == "-q")
		{
			verbose = false;
		}else if (arg == "-b")
		{
			if (i + 1 < argc)
			{
				try{
					POWER_BASELINE = std::stoul (argv[++i]);
				} catch (std::exception const &e)
				{
					printHelp();
				}
			}else
			{
				printHelp();
			}
		}
#endif
		else if (arg == "-e")
		{
			if (i + 1 < argc)
			{
				energyMode = std::string(argv[++i]);
			}else
			{
				printHelp();
			}
		}
		else
		{
			printHelp();
		}
	}
	if (energyMode == "" || (energyMode != "RAPL" && energyMode != "NVML"))
		printHelp();
	
////////////////////////////////////////////////////////////////////////////////	
	Environment::init(SYS_OTHER, defaultParameter, 0);
	
	if (energyMode == "RAPL")
	{
		measureEnergy<RAPL>(verbose);
	}
	else if (energyMode == "NVML")
	{
		measureEnergy<NVML>(verbose);
	}
	
	Environment::close();

	return 0;
}
