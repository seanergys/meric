#include "../../wrapper/environmentwrapper.h"
#include "../../wrapper/raplwrapper.h"
#include "../../wrapper/nvmlwrapper.h"
#include "../../wrapper/a64fxwrapper.h"
#include "../../wrapper/hwmonwrapper.h"
#include "../../basis/utils.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits>
#include <csignal>
#include <unistd.h>
#include <errno.h>
#include <iomanip>

using namespace meric;

int POWER_BASELINE = 0;
int job_id; // id specific to the execution.

//----------------------------------------------------------------------------//

template <typename Tmode>
struct EnergyRecords
{
	static std::vector<std::map<std::string, unsigned long long int>> recordEnergy;
	static std::vector<double> recordTime;

	static void printValues(bool verbose)
	{
		if (recordTime.size() < 2)
		{
			std::cerr << "ERROR missing samples\n";
			exit(2);
		}

		std::map<std::string, double> record;
		for (auto &kv : recordEnergy[0])
		{
			record[kv.first] = 0;
		}

		double summary = 0.0;
		double runtime = recordTime.back() - recordTime[0];

		for (int i = 1; i < recordEnergy.size(); i++)
		{
			for (auto &kv : record)
			{
				int cumulative = 0;
				double delta = Tmode::getResultValue(
					recordEnergy[i - 1][kv.first], recordEnergy[i][kv.first],
					recordTime[i - 1] - recordTime[i], cumulative, kv.first);
				record[kv.first] += delta;
				if (cumulative > 0)
				{
					summary += delta;
				}
			}
		}
		summary += POWER_BASELINE * runtime;

		if (verbose)
		{
			std::cout << "Runtime [s] = " << std::fixed << runtime << std::endl; // TODO: prevent scientific format, that's with e234 etc.

			// Print each counter value
			for (auto &kv : recordEnergy[0])
			{
				std::cout << Tmode::getCounterLabel(kv.first) << " = " << std::fixed << std::setprecision(3) << record[kv.first] << std::endl;
			}
			if (Tmode::getSummaryCounterName() != "")
			{
				std::cout << std::endl
						  << std::fixed << std::setprecision(3) << Tmode::getSourceLabel() << " " << Tmode::getSummaryCounterName() << " = " << summary << std::endl;
				std::cout << std::fixed << std::setprecision(3) << Tmode::getSourceLabel() << " " << "Energy consumption [Wh] = " << (summary / 3600.0) << "\n\n";
			}
		}
		else
		{
			std::cout << std::fixed << std::setprecision(3) << summary << std::endl; // TODO: prevent scientific format, that's with e234 etc.
		}
	}

	static void clear()
	{
		recordEnergy.clear();
		recordTime.clear();
	}

	static std::vector<std::map<std::string, unsigned long long int>> getData()
	{
		return recordEnergy;
	}
};

template <typename Tmode>
std::vector<std::map<std::string, unsigned long long int>> EnergyRecords<Tmode>::recordEnergy;

template <typename Tmode>
std::vector<double> EnergyRecords<Tmode>::recordTime;

//----------------------------------------------------------------------------//
//----------------------------------------------------------------- MAIN -----//
//----------------------------------------------------------------------------//

template <typename Tmode>
void measureEnergy(bool verbose, double timeNow = 0.0)
{
	if (EnergyRecords<Tmode>::recordEnergy.empty())
	{
		Tmode::init(true);
	}

	if (timeNow == 0.0)
	{
		struct timespec timestamp;
		clock_gettime(CLOCK_REALTIME, &timestamp);
		timeNow = mutils::timeToDouble(timestamp);
	}

	std::map<std::string, unsigned long long int> energyNow;
	Tmode::getCounterValues(energyNow);

	EnergyRecords<Tmode>::recordEnergy.push_back(energyNow);
	EnergyRecords<Tmode>::recordTime.push_back(timeNow);
}

struct HWMON_S
{
	static timespec startTime; // this is dummy.
	static timespec endTime;   // this is dummy.
}; // dummy class to seperate from counting.

timespec HWMON_S::startTime = {0, 0};
timespec HWMON_S::endTime = {0, 0};

// following will be called many times but init only once.
template <>
void measureEnergy<HWMON_S>(bool verbose, double timeNow)
{
	static bool is_init = false;
	if (is_init == false)
	{
		is_init = true;
		HWMON::init(true);
		HWMON::fillTime(HWMON_S::startTime);
		HWMON::start(HWMON_S::startTime);
	}
}

template <>
struct EnergyRecords<HWMON_S>
{
	static void printValues(bool verbose)
	{
		HWMON::fillTime(HWMON_S::endTime);
		HWMON::stop(HWMON_S::endTime);

		std::vector<double> energy(HWMON::vrSensorLabel.size(), 0);
		std::vector<double> minWatts(HWMON::vrSensorLabel.size(), 0);
		std::vector<double> maxWatts(HWMON::vrSensorLabel.size(), 0);

		size_t firstBlade, lastBlade, firstVR, lastVR;
		HWMON::getData(HWMON_S::startTime.tv_nsec, (HWMON_S::endTime.tv_nsec - HWMON_S::startTime.tv_nsec),
					   &firstBlade, &lastBlade, &firstVR, &lastVR, energy.data(), minWatts.data(), maxWatts.data());

		std::cout << "Runtime [s] = " << std::fixed << std::setprecision(3) << (HWMON_S::endTime.tv_nsec - HWMON_S::startTime.tv_nsec) / 1000000.0 << "\n";
		double summary = 0.0;
		for (auto &&i : energy)
		{
			summary += i;
		}
		std::cout << std::endl
						  << std::fixed << std::setprecision(3) << HWMON::getSourceLabel() << " " << HWMON::getSummaryCounterName() << " = " << summary << std::endl;
				std::cout << std::fixed << std::setprecision(3) << HWMON::getSourceLabel() << " " << "Energy consumption [Wh] = " << (summary / 3600.0) << "\n\n";

		HWMON::close();
	}
};

void printHelp()
{
	std::cerr << "\nMERIC static tool help:\n\n"
			  // TODO	          << "Run energyMeasureStart to start the energy measurement and change system parameters\n"
			  // TODO	          << "Run energyMeasureStop to stop energy measurement, restore system settings and print the result energy consumption to std::cout\n"
			  // TODO	          << "\nenergy measurement mode (-e) is the only mandatory parameter\n\n"
			  << "\t mericStatic [mericStatic parameters] -- ./application [application parameters]\n"
			  << "\t mericStatic [mericStatic parameters] -- start: Starts measuring energy untill mericStatic -- stop command issued.\n"
			  << "\t mericStatic [mericStatic parameters] -- stop: Sends SIGTERM signal to stop mericStatic commands.\n"
			  << "\t mericStatic [mericStatic parameters] -- eval: Reads all dump files and aggregates total energy consumption.\n"
			  << "\t mericStatic parameters:\n"
			  //	          << "\t-e = comma separated list of energy measurement systems: RAPL,NVML,A64FX\n"
			  << "\t-i = id of the execution. It is used to seperate measurements. if not specified by default 0.\n"
			  << "\t-e = one energy measurement systems from supported: RAPL, NVML, HWMON, or A64FX\n"
			  << "\t-c = CPU core frequency [Hz]\n"	  //, specify value with units [GHz, MHz, kHz or default Hz]\n"
			  << "\t-u = CPU uncore frequency [Hz]\n" //, specify value with units [GHz, MHz, kHz or default Hz]\n"
			  << "\t-t = #OpenMP threads\n"
			  << "\t-p = CPU power capping power limit [mW]\n"
			  << "\t-w = CPU power capping time window [ms]\n"
			  << "\t-s = GPU SM frequency [Hz]\n"
			  << "\t-r = GPU memory frequency [Hz]\n"
			  << "\t-g = GPU power capping power limit [mW]\n"
			  << "\t-b = single node baseline (static) power consumption [W]\n"
			  << "\t-q = print the overall consumed energy only [J]\n"
			  << "\t-o = output directory, ./mericStaticMeasurements by default\n"
			  << std::endl;
	exit(1);
}

bool isJobRunning(const std::string &jobName);
std::vector<std::string> listMatchingFiles(const std::string &prefix, const std::string &directory = ".");
std::map<std::string, double> extractEnergyConsumption(const std::string &filepath);
double extractMaxRuntime(const std::string &filepath);
bool createDirectoryIfNotExists(const std::string &path);

enum class TOOL_MODE
{
	DEFAULT,
	START,
	STOP,
	EVAL
};
TOOL_MODE toolMode = TOOL_MODE::DEFAULT;
bool isRunning = true;
int child_pid = -1;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		// write(STDOUT_FILENO, "Interrupt signal (Ctrl+C) received\n", 36);

		if (child_pid > 0)
		{
			kill(child_pid, SIGINT); // Forward signal to child process
									 // write(STDOUT_FILENO, "Signal forwarded to the child process\n", 39);
		}
		else
		{
			// write(STDOUT_FILENO, "Child not exist, terminating.\n", 31);
			isRunning = false;
			// _exit(0);
		}
	}
	else if (signal == SIGTERM)
	{
		// write(STDOUT_FILENO, "Termination signal received.\n", 30);

		if (child_pid > 0)
		{
			kill(child_pid, SIGTERM); // Forward signal to child process
									  // write(STDOUT_FILENO, "Signal forwarded to the child process\n", 39);
		}
		else
		{
			// write(STDOUT_FILENO, "Child not exist, terminating.\n", 31);
			isRunning = false;
			// _exit(0);
		}
	}
}

// Setup signal handler
void setupSignalHandlers()
{
	std::signal(SIGINT, signalHandler);	 // Ctrl+C
	std::signal(SIGTERM, signalHandler); // `kill` default
}

int main(int argc, char **argv)
{
	bool verbose = true;
	char hostname[HOST_NAME_MAX];
	std::string energyMode = "";
	std::string applicationCommand = "";
	std::string outputDirectory = "mericStaticMeasurements";

	const char *job_id_env = std::getenv("SLURM_JOB_ID") ? std::getenv("SLURM_JOB_ID") : std::getenv("PBS_JOBID") ? std::getenv("PBS_JOBID")
																					 : std::getenv("FLUX_JOB_ID") ? std::getenv("FLUX_JOB_ID")
																												  : "0";
	if (job_id_env)
		job_id = std::atoi(job_id_env);
	printf("job_id :%d\n", job_id);

	if (gethostname(hostname, HOST_NAME_MAX) != 0)
	{
		std::cerr << "missing machine hostname\n";
		exit(1);
	}
	setupSignalHandlers();
	RegionParameters defaultParameter;
	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg == "-c")
		{
			if (i + 1 < argc)
			{
				try
				{
					defaultParameter.frequency = std::stol(argv[++i]);
				}
				catch (std::exception const &e)
				{
					printHelp();
				}
			}
			else
			{
				printHelp();
			}
		}
		else if (arg == "-u")
		{
			if (i + 1 < argc)
			{
				try
				{
					defaultParameter.uncoreFrequency = std::stol(argv[++i]);
				}
				catch (std::exception const &e)
				{
					printHelp();
				}
			}
			else
			{
				printHelp();
			}
		}
		else if (arg == "-t")
		{
			if (i + 1 < argc)
			{
				try
				{
					defaultParameter.nthreads = std::stoi(argv[++i]);
				}
				catch (std::exception const &e)
				{
					printHelp();
				}
			}
			else
			{
				printHelp();
			}
		}
		else if (arg == "-p")
		{
			if (i + 1 < argc)
			{
				try
				{
					defaultParameter.powerCapPKG = std::stol(argv[++i]);
				}
				catch (std::exception const &e)
				{
					printHelp();
				}
			}
			else
			{
				printHelp();
			}
		}
		else if (arg == "-w")
		{
			if (i + 1 < argc)
			{
				try
				{
					defaultParameter.powerCapPKGtime = std::stol(argv[++i]);
				}
				catch (std::exception const &e)
				{
					printHelp();
				}
			}
			else
			{
				printHelp();
			}
		}
		else if (arg == "-s")
		{
			if (i + 1 < argc)
			{
				try
				{
					defaultParameter.frequencyGPU = std::stol(argv[++i]);
				}
				catch (std::exception const &e)
				{
					printHelp();
				}
			}
			else
			{
				printHelp();
			}
		}
		else if (arg == "-r")
		{
			if (i + 1 < argc)
			{
				try
				{
					defaultParameter.frequencyGPUmem = std::stol(argv[++i]);
				}
				catch (std::exception const &e)
				{
					printHelp();
				}
			}
			else
			{
				printHelp();
			}
		}
		else if (arg == "-g")
		{
			if (i + 1 < argc)
			{
				try
				{
					defaultParameter.powerCapGPU = std::stol(argv[++i]);
				}
				catch (std::exception const &e)
				{
					printHelp();
				}
			}
			else
			{
				printHelp();
			}
		}
		else if (arg == "-q")
		{
			verbose = false;
		}
		else if (arg == "-b")
		{
			if (i + 1 < argc)
			{
				try
				{
					POWER_BASELINE = std::stoul(argv[++i]);
				}
				catch (std::exception const &e)
				{
					printHelp();
				}
			}
			else
			{
				printHelp();
			}
		}
		else if (arg == "-e")
		{
			if (i + 1 < argc)
			{
				energyMode = std::string(argv[++i]);
				std::transform(energyMode.begin(), energyMode.end(), energyMode.begin(), ::toupper);
			}
			else
			{
				printHelp();
			}
		}
		else if (arg == "-i")
		{
			if (i + 1 < argc)
			{
				job_id = std::atoi(argv[++i]);
			}
			else
			{
				printHelp();
			}
		}
		else if (arg == "-o")
		{
			if (i + 1 < argc)
			{
				outputDirectory = std::string(argv[++i]);
			}
			else
			{
				printHelp();
			}
		}
		else if (arg == "--")
		{
			if (i + 1 < argc)
			{
				i++;

				if (std::string(argv[i]) == "start")
				{
					toolMode = TOOL_MODE::START;
				}
				else if (std::string(argv[i]) == "stop")
				{
					toolMode = TOOL_MODE::STOP;
					applicationCommand = "pkill -u \"$USER\" -f mericStatic";
				}
				else if (std::string(argv[i]) == "eval")
				{
					toolMode = TOOL_MODE::EVAL;
				}
				else
				{
					toolMode = TOOL_MODE::DEFAULT;
					for (; i < argc; i++)
					{
						applicationCommand += " " + std::string(argv[i]);
					}
				}
			}
			else
				printHelp();
		}
		else
		{
			printHelp();
		}
	}

	std::ofstream outfile;
	std::streambuf *coutbuf = std::cout.rdbuf(); // Save original buffer
	if (toolMode == TOOL_MODE::DEFAULT || toolMode == TOOL_MODE::START)
	{
		createDirectoryIfNotExists(outputDirectory);

		// redirect the output to the file created.
		std::string filename = outputDirectory + "/" + "mericStatic_" + std::to_string(job_id) + "_" + hostname + ".data";
		outfile.open(filename, std::ios::out | std::ios::trunc);
		if (outfile.is_open())
			std::cout.rdbuf(outfile.rdbuf()); // Redirect cout to file
		else
			std::cerr << "Error creating the file:" << filename << "\n";
	}

	std::vector<void (*)(bool, double)> measurements;
	std::vector<void (*)(bool)> prints;

	if ((toolMode == TOOL_MODE::DEFAULT || toolMode == TOOL_MODE::START) && energyMode == "")
	{
		printHelp();
	}
	else
	{
		while (energyMode != "")
		{
			std::string token;
			size_t pos = energyMode.find_first_of(',');
			if (pos != std::string::npos)
			{
				token = energyMode.substr(0, pos);
				energyMode = energyMode.substr(pos + 1, energyMode.size() - pos);
			}
			else
			{
				token = energyMode;
				energyMode = "";
			}

			if (token == "RAPL")
			{
				measurements.push_back(&measureEnergy<RAPL>);
				prints.push_back(EnergyRecords<RAPL>::printValues);
			}
			else if (token == "NVML")
			{
				measurements.push_back(&measureEnergy<NVML>);
				prints.push_back(EnergyRecords<NVML>::printValues);
			}
			else if (token == "A64FX")
			{
				measurements.push_back(&measureEnergy<A64FX>);
				prints.push_back(EnergyRecords<A64FX>::printValues);
			}
			else if (token == "HWMON_C") // counting mode
			{
				measurements.push_back(&measureEnergy<HWMON>);
				prints.push_back(EnergyRecords<HWMON>::printValues);
			}
			else if (token == "HWMON_S") // sampling mode for grace11
			{
				measurements.push_back(&measureEnergy<HWMON_S>);
				prints.push_back(EnergyRecords<HWMON_S>::printValues);
			}
			else
			{
				std::cerr << "ERROR: unknown energy measurement system '" + token + "'\n\n";
				printHelp();
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	Environment::init(SYS_OTHER, defaultParameter, 0);

	double endTime = 0.0;
	struct timespec timestamp;
	clock_gettime(CLOCK_REALTIME, &timestamp);
	double timeNow = mutils::timeToDouble(timestamp);
	for (auto measureEnergyGeneral : measurements)
	{
		measureEnergyGeneral(verbose, timeNow);
	}

	if (toolMode == TOOL_MODE::DEFAULT && !applicationCommand.empty())
	{
		int pid = fork();
		if (pid == 0) // child
		{
			// Replace the child process with the application command
			char *const args[] = {(char *)"sh", (char *)"-c", (char *)applicationCommand.c_str(), nullptr};
			execvp(args[0], args); // Replace with the command, use execvp to search in PATH

			// If execvp fails, print an error
			std::cerr << "Error: " << strerror(errno) << std::endl;
			exit(1);
		}
		else if (pid > 0) // parent
		{
			child_pid = pid;
			double waitTime = timeNow;

			// Parent checks periodically if the child has finished
			while (isRunning)
			{
				int status;
				pid_t result = waitpid(child_pid, &status, WNOHANG); // Non-blocking check for child status

				if (result == -1)
				{
					std::cerr << "Error: " << strerror(errno) << std::endl;
					exit(1);
				}
				else if (result == 0)
				{
					// Child has not finished yet, keep checking
					clock_gettime(CLOCK_REALTIME, &timestamp);
					timeNow = mutils::timeToDouble(timestamp);

					if (timeNow - waitTime > 60) // TODO: application input parameter
					{
						for (auto measureEnergyGeneral : measurements)
						{
							measureEnergyGeneral(verbose, timeNow);
						}
						waitTime = timeNow;
					}

					usleep(1000); // Sleep for 1 second before checking again
				}
				else
				{
					// Child has finished
					child_pid = -1; // reset the child_pid.
					clock_gettime(CLOCK_REALTIME, &timestamp);
					endTime = mutils::timeToDouble(timestamp);
					isRunning = false;
				}
			}

			// After the child finishes, measure energy and print
			if (endTime > 0.0)
			{
				// Final energy measurement stamp
				for (auto measureEnergyGeneral : measurements)
				{
					measureEnergyGeneral(verbose, endTime);
				}

				for (auto printGeneral : prints)
				{
					printGeneral(verbose);
				}
			}
		}
		else
		{ // error
			std::cerr << "Fork failed: " << strerror(errno) << std::endl;
			exit(1);
		}
	}
	else if (toolMode == TOOL_MODE::START)
	{
		double waitTime = timeNow;
		while (isRunning)
		{
			clock_gettime(CLOCK_REALTIME, &timestamp);
			timeNow = mutils::timeToDouble(timestamp);

			if (timeNow - waitTime > 60) // TODO: application input parameter
			{
				for (auto measureEnergyGeneral : measurements)
				{
					measureEnergyGeneral(verbose, timeNow);
				}
				waitTime = timeNow;
			}

			usleep(1000); // Sleep for 1 second before checking again
		}
		clock_gettime(CLOCK_REALTIME, &timestamp);
		endTime = mutils::timeToDouble(timestamp);

		// After the child finishes, measure energy and print
		if (endTime > 0.0)
		{
			// Final energy measurement stamp
			for (auto measureEnergyGeneral : measurements)
			{
				measureEnergyGeneral(verbose, endTime);
			}

			for (auto printGeneral : prints)
			{
				printGeneral(verbose);
			}
		}
	}
	else if (toolMode == TOOL_MODE::STOP && !applicationCommand.empty())
	{
		if (std::system(applicationCommand.c_str()) != 0)
			std::cerr << "MERIC static ERROR: could not run `" << applicationCommand << "`\n";
	}
	else if (toolMode == TOOL_MODE::EVAL)
	{

		if (isJobRunning("mericStatic"))
			std::cout << "Job is not yet completed!\n";
		else
		{

			// check if all files are there, if so aggregate the total energy
			std::string prefix = "mericStatic_" + std::to_string(job_id);
			std::vector<std::string> files = listMatchingFiles(prefix, outputDirectory);
			std::map<std::string, double> totalEnergy = {
				{"RAPL", 0.0},
				{"NVML", 0.0},
				{"A64FX", 0.0},
				{"HWMON", 0.0},
				{"ROCM", 0.0},
			};
			double maxRuntime = 0.0;
			for (const auto &filepath : files)
			{
				auto energies = extractEnergyConsumption(filepath);
				for (const auto &entry : energies)
					totalEnergy[entry.first] += entry.second;

				double localRuntime = extractMaxRuntime(filepath);
				maxRuntime = localRuntime > maxRuntime ? localRuntime : maxRuntime;
				// std::cout << filepath << "\n";
			}
			std::cout << "Max Runtime [s] = " << std::fixed << std::setprecision(3) << maxRuntime << "\n";
			// std::cout << "Total Energy consumption [J] = " << totalEnergy << "\n";
			double totalSum = 0;
			for (const auto &entry : totalEnergy)
			{
				if (entry.second == 0.0)
					continue;
				std::cout << entry.first << " Energy consumption [J] = " << std::fixed << std::setprecision(3) << entry.second << "\n";
				totalSum += entry.second;
			}
			std::cout << "\n";
			std::cout << "Total Energy consumption [J] = " << std::fixed << std::setprecision(3) << totalSum << "\n";
			std::cout << "Total Energy consumption [Wh] = " << std::fixed << std::setprecision(3) << (totalSum / 3600.0) << "\n";
		}
	}

	Environment::restoreSystemSettings();
	Environment::close();

	if (outfile.is_open())
	{
		std::cout.rdbuf(coutbuf);
		outfile.close();
	}

	return 0;
}
double extractMaxRuntime(const std::string &filepath)
{
	std::ifstream file(filepath);
	std::string line;
	std::regex runtimeRegex(R"(Runtime \[s\] = ([0-9.]+))");
	std::smatch match;

	double maxRuntime = 0.0;

	while (std::getline(file, line))
	{
		if (std::regex_search(line, match, runtimeRegex))
		{
			double runtime = std::stod(match[1]);
			if (runtime > maxRuntime)
			{
				maxRuntime = runtime;
			}
		}
	}
	return maxRuntime; // Will be 0.0 if not found
}

std::map<std::string, double> extractEnergyConsumption(const std::string &filepath)
{
	std::ifstream file(filepath);
	std::string line;

	std::map<std::string, double> energyValues = {
		{"RAPL", 0.0},
		{"NVML", 0.0},
		{"A64FX", 0.0},
		{"HWMON", 0.0},
		{"ROCM", 0.0},
	};

	std::regex pattern(R"((RAPL|NVML|A64FX|HWMON|ROCM)\s+Energy consumption\s+\[J\]\s+=\s+([0-9.]+))");
	std::smatch match;

	while (std::getline(file, line))
	{
		if (std::regex_search(line, match, pattern))
		{
			std::string source = match[1];
			double value = std::stod(match[2]);
			energyValues[source] = value;
		}
	}

	return energyValues;
}

std::vector<std::string> listMatchingFiles(const std::string &prefix, const std::string &directory)
{
	std::vector<std::string> result;
	DIR *dir = opendir(directory.c_str());
	if (!dir)
		return result;

	struct dirent *entry;
	while ((entry = readdir(dir)) != nullptr)
	{
		std::string name(entry->d_name);
		if (name.find(prefix) == 0)
		{
			result.push_back(directory + "/" + name);
		}
	}
	closedir(dir);
	return result;
}
bool isJobRunning(const std::string &jobName)
{
	std::string cmd = "srun ps -u $USER -o args= | grep " + jobName + " | grep -v grep | grep -v ' -- eval'";

	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
	if (!pipe)
	{
		std::cerr << "Failed to run command.\n";
		return false;
	}

	char buffer[256];
	std::string result;

	while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr)
	{
		result += buffer;
	}
	// std::cout << result << "\n";
	return !result.empty();
}

bool createDirectoryIfNotExists(const std::string &path)
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
	{
		// Directory does not exist
		if (mkdir(path.c_str(), 0755) != 0)
		{
			std::cerr << "Failed to create directory '" << path << "': " << strerror(errno) << "\n";
			return false;
		}
	}
	else if (!S_ISDIR(st.st_mode))
	{
		std::cerr << "'" << path << "' exists but is not a directory.\n";
		return false;
	}
	return true;
}