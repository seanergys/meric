
#include "../../wrapper/environmentwrapper.h"
#include "../../wrapper/raplwrapper.h"

using namespace meric;

std::string OUTPUT_FILE;
int RAPL_BASELINE = 0;
#ifdef START
void storeValues(std::map<std::string, unsigned long long int> &record, double time);
#endif

#ifdef STOP
double readStoredValues(std::map<std::string, unsigned long long int> & oldRecord);
void printValues(std::map<std::string, unsigned long long int> &oldRecord, std::map<std::string, unsigned long long int> &newRecord, double runtime, bool verbose);
#endif

//----------------------------------------------------------------------------//
#ifdef START
void storeValues(std::map<std::string, unsigned long long int> &record, double time)
{
	std::remove(OUTPUT_FILE.c_str());
	std::ofstream ss (OUTPUT_FILE, std::ofstream::out);
	//store time
	ss << std::setprecision(std::numeric_limits<double>::digits10 + 1) << time << std::endl;
	//store counters
	for(std::map<std::string, unsigned long long int>::iterator iterator = record.begin(); iterator != record.end(); iterator++)
	{
		if (RAPL::getSummaryCounterName() == iterator->first)
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

void printValues(std::map<std::string, unsigned long long int> &oldRecord, std::map<std::string, unsigned long long int> &newRecord, double runtime, bool verbose)
{
	std::map<std::string, unsigned long long int> record;
	double summary = 0.0;
	
	if (verbose)
	{
		std::cout << "Runtime [s] = " << runtime << std::endl;
		for(std::map<std::string, unsigned long long int>::iterator iterator = oldRecord.begin(); iterator != oldRecord.end(); iterator++)
		{
			double value = RAPL::getResultValue(oldRecord[iterator->first], newRecord[iterator->first], runtime, iterator->first);
			std::cout << RAPL::getCounterLabel(iterator->first) << " = " << value << std::endl;
			summary += value;
		}
		summary += RAPL_BASELINE*runtime;
		std::cout << std::endl << RAPL_SUM_ENERGY << " = " << summary << std::endl;
	}else
	{
		for(std::map<std::string, unsigned long long int>::iterator iterator = oldRecord.begin(); iterator != oldRecord.end(); iterator++)
		{
			summary += RAPL::getResultValue(oldRecord[iterator->first], newRecord[iterator->first], runtime, iterator->first);
		}
		std::cout <<  summary + RAPL_BASELINE*runtime << std::endl;
	}
}
#endif

//----------------------------------------------------------------------------//
//----------------------------------------------------------------- MAIN -----//
//----------------------------------------------------------------------------//
void printHelp()
{
	std::cerr << "\nMERIC static tool help:\n\n"
	          << "Run energyMeasureStart to start the energy measurement (using RAPL) and change system parameters\n"
	          << "Run energyMeasureStop to stop energy measurement, restore system settings and print the result energy consumption to std::cout\n"
	          << "\nenergyMeasureStart parameters:\n"
	          << "\t-c = CPU core frequency [Hz]\n" //, specify value with units [GHz, MHz, kHz or default Hz]\n"
	          << "\t-u = CPU uncore frequency [Hz]\n" //, specify value with units [GHz, MHz, kHz or default Hz]\n"
	          << "\t-t = #OpenMP threads\n"
	          << "\t-p = CPU power capping power limit [mW]\n"
	          << "\t-w = CPU power capping time window [ms]\n"
	          << "\t-s = GPU SM frequency [Hz]\n"
	          << "\t-r = GPU memory frequency [Hz]\n"
	          << "\t-g = GPU power capping power limit [mW]\n"
	          << "\nenergyMeasureStop parameters:\n"
	          << "\t-b = node baseline (static) power [W]\n"
	          << "\t-q = print the overall consumed energy only [J]\n"
	          << std::endl;
	exit(1);
}

int main(int argc, char **argv)
{
#ifdef STOP
	bool verbose = true;
#endif
	char hostname[HOST_NAME_MAX];
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
					RAPL_BASELINE = std::stoul (argv[++i]);
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
		else
		{
			printHelp();
		}
	}
////////////////////////////////////////////////////////////////////////////////	
	Environment::init(SYS_OTHER, defaultParameter, 0);
	RAPL::init(true);

	struct timespec timeNow;
	clock_gettime(CLOCK_REALTIME, &timeNow);

	std::map<std::string, unsigned long long int> recordNow;
	RAPL::getCounterValues(recordNow);

#ifdef START
	storeValues(recordNow, mutils::timeToDouble(timeNow));
#endif

#ifdef STOP
	std::map<std::string, unsigned long long int> recordOld;
	double timeOld = readStoredValues(recordOld);
	printValues(recordOld, recordNow, mutils::timeToDouble(timeNow)-timeOld, verbose);
	Environment::restoreSystemSettings(true);
#endif
	Environment::close();

	return 0;
}
