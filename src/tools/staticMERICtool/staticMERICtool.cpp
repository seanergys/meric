#include "../../wrapper/environmentwrapper.h"
#include "../../wrapper/raplwrapper.h"
#include "../../wrapper/nvmlwrapper.h"
#include "../../wrapper/a64fxwrapper.h"
#include "../../wrapper/hwmonwrapper.h"
#include "../../wrapper/rocmwrapper.h"
#include "../../basis/utils.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <limits>
#include <csignal>
#include <iomanip>

using namespace meric;

int POWER_BASELINE = 0;
int measurement_id; // id specific to the execution.

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
			std::cerr << "MERIC ERROR: Missing measurement samples\n";
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
			std::cout << "Runtime [s] = " << std::fixed << std::setprecision(3) << runtime << std::endl;

			// Print each counter value
			for (auto &kv : recordEnergy[0])
			{
				std::cout << Tmode::getCounterLabel(kv.first) << " = " << std::fixed << std::setprecision(3) << record[kv.first] << std::endl;
			}
			if (Tmode::getSummaryCounterName() != "")
			{
				std::cout << std::endl
						  << std::fixed << std::setprecision(3) << Tmode::getSourceLabel() << " " << Tmode::getSummaryCounterName() << " = " << summary << "\n"
						  << std::fixed << std::setprecision(3) << Tmode::getSourceLabel() << " " << "Energy consumption [Wh] = " << (summary / 3600.0) << "\n\n";
			}
		}
		else
		{
			std::cout << std::fixed << std::setprecision(3) << summary << std::endl;
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

template <>
void measureEnergy<ROCM>(bool verbose, double timeNow)
{
	if (EnergyRecords<ROCM>::recordEnergy.empty())
	{
		ROCM::init(true);
	}

	if (timeNow == 0.0)
	{
		struct timespec timestamp;
		clock_gettime(CLOCK_REALTIME, &timestamp);
		timeNow = mutils::timeToDouble(timestamp);
	}

	std::map<std::string, uint64_t> energyNow;
	ROCM::getCounterValues(energyNow);

	// Convert int64_t map to unsigned long long int map for compatibility
	std::map<std::string, unsigned long long int> energyConverted;
	for (const auto &kv : energyNow)
	{
		energyConverted[kv.first] = static_cast<unsigned long long int>(kv.second);
	}

	EnergyRecords<ROCM>::recordEnergy.push_back(energyConverted);
	EnergyRecords<ROCM>::recordTime.push_back(timeNow);
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

		double duration = (HWMON_S::endTime.tv_sec - HWMON_S::startTime.tv_sec) + (HWMON_S::endTime.tv_nsec - HWMON_S::startTime.tv_nsec) / 1.0e9;

		std::vector<double> energy(HWMON::PROBE_NUMBER, 0);
		std::vector<double> minWatts(HWMON::PROBE_NUMBER, 0);
		std::vector<double> maxWatts(HWMON::PROBE_NUMBER, 0);

		size_t firstBlade, lastBlade, firstVR, lastVR;
		HWMON::getData(HWMON_S::startTime.tv_sec + HWMON_S::startTime.tv_nsec / 1.0e9, duration,
					   &firstBlade, &lastBlade, &firstVR, &lastVR, energy.data(), minWatts.data(), maxWatts.data());

		std::cout << "Runtime [s] = " << std::fixed << duration << "\n";
		// double summary = 0.0;
		// for (auto &&i : energy)
		// {
		// 	summary += (i / (lastVR - firstVR) * duration);
		// }

		double summary = 0.0;
		double avgWatts = 0.0;
		double energyConsumption = 0.0;
		for (unsigned int sensorID = 1; sensorID < HWMON::vrSensorLabel.size(); sensorID++)
		{
			if (HWMON::vrSensorLabel[sensorID].find("Grace_Power_Socket") == std::string::npos)
			{
				continue;
			}

			avgWatts = energy[sensorID] / (lastVR - firstVR);
			energyConsumption = duration * avgWatts;
			summary += energyConsumption;

			// std::cout << "sensorID:" << sensorID << "\n";
			// std::cout << "# SAMPLES - " << HWMON::getSourceLabel() << " (" << HWMON::vrSensorLabel[sensorID] << "):\n"
			// 		  << HWMON::SUM_ENERGY_LABEL << "," << std::fixed << std::setprecision(6) << energyConsumption << '\n'
			// 		  << "MAX power [W]" << "," << maxWatts[sensorID] << '\n'
			// 		  << "MIN power [W]" << "," << minWatts[sensorID] << '\n'
			// 		  << "AVG power [W]" << "," << avgWatts << '\n'
			// 		  << "ID of first sample" << "," << firstVR << '\n'
			// 		  << "ID of last sample" << "," << lastVR - 1 << std::endl
			// 		  << std::endl;
		}
		std::cout << '\n'
				  << std::fixed << std::setprecision(3) << HWMON::getSourceLabel() << " " << HWMON::getSummaryCounterName() << " = " << summary << '\n'
				  << std::fixed << std::setprecision(3) << HWMON::getSourceLabel() << " " << "Energy consumption [Wh] = " << (summary / 3600.0) << '\n'
				  << std::endl;

		HWMON::close();
	}
};

void printHelp()
{
	std::cerr << "\nMERIC static tool help:\n\n"
			  << "\t mericStatic [mericStatic parameters] -- ./application [application parameters]\n"
			  << "\t mericStatic [mericStatic parameters] -- start: Starts measuring energy untill mericStatic -- stop command issued.\n"
			  << "\t mericStatic [mericStatic parameters] -- stop: Sends SIGTERM signal to stop mericStatic commands.\n"
			  << "\t mericStatic [mericStatic parameters] -- eval: Reads all dump files and aggregates total energy consumption.\n"
			  << "\t mericStatic parameters:\n"
			  << "\t-e = comma separated list of energy measurement systems: RAPL, NVML, ROCM, HWMON-C, HWMON-S, or A64FX\n"
			  << "\t-i = id of the execution. It is used to seperate measurements. if not specified by default 0.\n"
			  << "\t-c = CPU core frequency [Hz]\n"	  //, specify value with units [GHz, MHz, kHz or default Hz]\n"
			  << "\t-u = CPU uncore frequency [Hz]\n" //, specify value with units [GHz, MHz, kHz or default Hz]\n"
			  << "\t-t = #OpenMP threads\n"
			  << "\t-T = Sampling period, default 60 in seconds.\n"
			  << "\t-p = CPU power capping power limit [mW]\n"
			  << "\t-w = CPU power capping time window [ms]\n"
			  << "\t-s = GPU SM frequency [Hz]\n"
			  << "\t-r = GPU memory frequency [Hz]\n"
			  << "\t-g = GPU power capping power limit [mW]\n"
			  << "\t-b = Baseline (static) power consumption [W]\n"
			  << "\t-q = print the overall consumed energy only [J]\n"
			  << "\t-o = output directory, ./mericStaticMeasurements by default\n"
			  << std::endl;
	exit(EXIT_FAILURE);
}
// TODO header file
std::vector<std::string> listMatchingFiles(const std::string &prefix, const std::string &directory = ".");
std::map<std::string, double> extractEnergyConsumption(const std::string &filepath);
double extractMaxRuntime(const std::string &filepath);

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
			// Forward signal to child process
			// write(STDOUT_FILENO, "Signal forwarded to the child process\n", 39);
			kill(child_pid, SIGINT);
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
			// Forward signal to child process
			// write(STDOUT_FILENO, "Signal forwarded to the child process\n", 39);
			kill(child_pid, SIGTERM);
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

	const char *measurement_id_env = std::getenv("SLURM_JOB_ID") ? std::getenv("SLURM_JOB_ID") : std::getenv("PBS_JOBID") ? std::getenv("PBS_JOBID")
																							 : std::getenv("FLUX_JOB_ID") ? std::getenv("FLUX_JOB_ID")
																														  : "0";
	if (measurement_id_env)
		measurement_id = std::atoi(measurement_id_env);
	printf("measurement_id :%d\n", measurement_id);

	if (gethostname(hostname, HOST_NAME_MAX) != 0)
	{
		std::cerr << "MERIC WARNING: gethostname failed (" << strerror(errno) << "), trying /etc/hostname\n";
		std::ifstream hostname_file("/etc/hostname");
		if (hostname_file.is_open())
		{
			std::string line;
			std::getline(hostname_file, line);
			std::strncpy(hostname, line.c_str(), HOST_NAME_MAX);
			hostname[HOST_NAME_MAX - 1] = '\0';
		}
		else
		{
			std::cerr << "MERIC ERROR: Cannot get hostname from /etc/hostname. Using fallback to \"unknown_host\".\n";
			std::strncpy(hostname, "unknown_host", HOST_NAME_MAX);
			hostname[HOST_NAME_MAX - 1] = '\0';
		}
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
		else if (arg == "-T")
		{
			if (i + 1 < argc)
			{
				try
				{
					defaultParameter.samplingPeriod = std::stoi(argv[++i]);
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
				measurement_id = std::atoi(argv[++i]);
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
			{
				printHelp();
			}
		}
		else
		{
			printHelp();
		}
	}

	// std::cout << "SAMPLING PERIOD ->" << defaultParameter.samplingPeriod << " s\n";

	std::ofstream outfile;
	std::streambuf *coutbuf = std::cout.rdbuf(); // Save original buffer
	if (toolMode == TOOL_MODE::DEFAULT || toolMode == TOOL_MODE::START)
	{
		meric::mutils::createDir(outputDirectory);

		// redirect the output to the file created.
		std::string filename = outputDirectory + "/" + "mericStatic_" + std::to_string(measurement_id) + "_" + hostname + ".data";
		outfile.open(filename, std::ios::out | std::ios::trunc);
		if (outfile.is_open())
		{
			std::cout.rdbuf(outfile.rdbuf()); // Redirect cout to a file
		}
		else
		{
			std::cerr << "MERIC ERROR: Cannot create file:" << filename << "\n";
			exit(EXIT_FAILURE);
		}
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
			else if (token == "HWMON-C") // counting mode
			{
				measurements.push_back(&measureEnergy<HWMON>);
				prints.push_back(EnergyRecords<HWMON>::printValues);
			}
			else if (token == "HWMON-S") // sampling mode
			{
				measurements.push_back(&measureEnergy<HWMON_S>);
				prints.push_back(EnergyRecords<HWMON_S>::printValues);
			}
			else if (token == "ROCM") // sampling mode
			{
				measurements.push_back(&measureEnergy<ROCM>);
				prints.push_back(EnergyRecords<ROCM>::printValues);
			}
			else
			{
				std::cerr << "MERIC ERROR: Unknown energy measurement system '" + token + "'\n\n";
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

			// If execvp fails, print an error and exit
			std::cerr << "MERIC ERROR: Failed to launch application command: \"" << applicationCommand << "\"\n";
			std::cerr << "Reason (errno " << errno << "): " << strerror(errno) << std::endl;

			switch (errno)
			{
			case E2BIG:
				std::cerr << "Hint: The command or its arguments are too long for the system to handle.\n";
				break;
			case EACCES:
				std::cerr << "Hint: Permission denied.\n"
						  << " - Ensure the shell or script is executable.\n"
						  << " - Check execute permissions using `chmod +x`.\n";
				break;
			case EFAULT:
				std::cerr << "Hint: Command or arguments point to an invalid memory location (corrupted input).\n";
				break;
			case EINVAL:
				std::cerr << "Hint: Invalid ELF executable or argument array. (Very rare for `sh -c`.)\n";
				break;
			case ENAMETOOLONG:
				std::cerr << "Hint: The command path is too long.\n";
				break;
			case ENOENT:
				std::cerr << "Hint: Shell ('sh') not found in PATH, or command inside 'sh -c' is invalid.\n"
						  << " - Verify '/bin/sh' exists.\n"
						  << " - Check spelling of the application command.\n";
				break;
			case ENOEXEC:
				std::cerr << "Hint: The file is not a valid executable format.\n"
						  << " - Possibly missing shebang (#!/bin/bash) in scripts.\n";
				break;
			case ENOMEM:
				std::cerr << "Hint: Not enough memory to run the process.\n"
						  << " - Try freeing memory or reducing load.\n";
				break;
			case ENOTDIR:
				std::cerr << "Hint: A component of the command path is not a directory.\n";
				break;
			case ELOOP:
				std::cerr << "Hint: Too many symbolic links in the command path.\n"
						  << " - Try resolving symlinks manually with `readlink -f`.\n";
				break;
			default:
				std::cerr << "Hint: Unhandled error. Refer to system documentation or use `strace` to debug.\n";
				break;
			}

			exit(EXIT_FAILURE);
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
					std::cerr << "MERIC ERROR: waitpid failed for child PID " << child_pid
							  << " (errno " << errno << "): " << strerror(errno) << std::endl;

					switch (errno)
					{
					case ECHILD:
						std::cerr << "Hint: No child process exists with the specified PID.\n"
								  << " - The process may have already terminated and been reaped.\n"
								  << " - Or the PID provided is not a child of the current process.\n";
						break;

					case EINTR:
						std::cerr << "Hint: The call was interrupted by a signal before any child exited.\n"
								  << " - This can happen if a signal handler (e.g., SIGINT or SIGCHLD) interrupted waitpid.\n"
								  << " - Consider retrying the call in a loop or using signal-safe handling.\n";
						break;

					case EINVAL:
						std::cerr << "Hint: Invalid arguments provided to waitpid.\n"
								  << " - The options may be incorrect or unsupported on this system.\n"
								  << " - Common mistake: misusing WNOHANG or combining incompatible flags.\n";
						break;

					case EFAULT:
						std::cerr << "Hint: The status pointer points to an invalid memory address.\n"
								  << " - This usually indicates memory corruption or an uninitialized pointer.\n"
								  << " - Check if '&status' is correctly defined and not a dangling pointer.\n";
						break;

					default:
						std::cerr << "Hint: Unknown error. errno value not recognized by standard waitpid errors.\n"
								  << " - Refer to the system's documentation or inspect surrounding code for issues.\n";
						break;
					}
					exit(EXIT_FAILURE);
				}
				else if (result == 0)
				{
					// Child has not finished yet, keep checking
					clock_gettime(CLOCK_REALTIME, &timestamp);
					timeNow = mutils::timeToDouble(timestamp);

					if (timeNow - waitTime > defaultParameter.samplingPeriod)
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
			exit(EXIT_FAILURE);
		}
	}
	else if (toolMode == TOOL_MODE::START)
	{
		double waitTime = timeNow;
		while (isRunning)
		{
			clock_gettime(CLOCK_REALTIME, &timestamp);
			timeNow = mutils::timeToDouble(timestamp);

			if (timeNow - waitTime > defaultParameter.samplingPeriod)
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
		{
			std::cerr << "MERIC ERROR: Could not run `" << applicationCommand << "`\n";
		}
	}
	else if (toolMode == TOOL_MODE::EVAL)
	{
		// check if all files are there, if so aggregate the total energy
		std::string prefix = "mericStatic_" + std::to_string(measurement_id);
		std::vector<std::string> files = listMatchingFiles(prefix, outputDirectory);
		std::map<std::string, double> totalEnergy = {
			{"RAPL", 0.0},
			{"NVML", 0.0},
			{"A64FX", 0.0},
			{"HWMON", 0.0}, // hwmon-s and hwmon-c are merged into HWMON since only one can be active.
			{"ROCM", 0.0},
		};
		double maxRuntime = 0.0;
		for (const auto &filepath : files)
		{
			auto energies = extractEnergyConsumption(filepath);
			for (const auto &entry : energies)
			{
				totalEnergy[entry.first] += entry.second;
			}

			double localRuntime = extractMaxRuntime(filepath);
			maxRuntime = localRuntime > maxRuntime ? localRuntime : maxRuntime;
		}
		std::cout << "Max Runtime [s] = " << std::fixed << std::setprecision(3) << maxRuntime << "\n";
		double totalSum = 0;
		for (const auto &entry : totalEnergy)
		{
			if (entry.second == 0.0)
			{
				continue;
			}
			std::cout << entry.first << " Energy consumption [J] = " << std::fixed << std::setprecision(3) << entry.second << "\n";
			totalSum += entry.second;
		}
		std::cout << "\n"
				  << "Total Energy consumption [J] = " << std::fixed << std::setprecision(3) << totalSum << "\n"
				  << "Total Energy consumption [Wh] = " << std::fixed << std::setprecision(3) << (totalSum / 3600.0) << "\n";
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
	return maxRuntime; // 0.0 if not found
}


std::map<std::string, double> extractEnergyConsumption(const std::string &filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open())
	{
		std::cerr << "MERIC ERROR: Unable to open energy file: " << filepath << "\n";
		exit(EXIT_FAILURE);
	}

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

	bool found = false;
	while (std::getline(file, line))
	{
		if (std::regex_search(line, match, pattern))
		{
			std::string source = match[1];
			double value = std::stod(match[2]);
			energyValues[source] = value;
			found = true;
		}
	}

	if (!found)
	{
		char hostname[HOST_NAME_MAX];
		if (gethostname(hostname, HOST_NAME_MAX) != 0)
		{
			std::strncpy(hostname, "unknown_host", HOST_NAME_MAX);
			hostname[HOST_NAME_MAX - 1] = '\0';
		}
		std::cerr << "MERIC ERROR: Job is not completed or energy data missing for: \n";
		std::cerr << "\tHostname: " << hostname << "\n";
		exit(EXIT_FAILURE);
	}

	return energyValues;
}


std::vector<std::string> listMatchingFiles(const std::string &prefix, const std::string &directory)
{
	std::vector<std::string> result;
	DIR *dir = opendir(directory.c_str());
	if (!dir)
	{
		return result;
	}

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
