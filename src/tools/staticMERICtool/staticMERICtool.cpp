#include "../../wrapper/environmentwrapper.h"
#include "../../wrapper/raplwrapper.h"
#include "../../wrapper/nvmlwrapper.h"
#include "../../wrapper/a64fxwrapper.h"
#include "../../wrapper/hwmonwrapper.h"
#include "../../basis/utils.h"
#include <limits>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>

using namespace meric;

int POWER_BASELINE = 0;
int job_id; // id specific to the execution.

//----------------------------------------------------------------------------//

std::vector<std::map<std::string, unsigned long long int>> recordEnergy; // TODO change to vector<vectors<unsigned long long int>> with mapping the strings to vector element IDs // TODO double
std::vector<double> recordTime;

template <typename Tmode>
void printValues(bool verbose)
{
	//	std::cout << "#SAMPLES: " << recordTime.size() << '\n'
	//	          << "START: " << recordTime[0] << '\n'
	//	          << "STOP: " << recordTime.back() << '\n';

	if (recordTime.size() < 2)
	{
		std::cerr << "ERROR missing samples\n";
		exit(2);
	}

	std::map<std::string, double> record;
	for (std::map<std::string, unsigned long long int>::iterator iterator = recordEnergy[0].begin(); iterator != recordEnergy[0].end(); iterator++)
	{
		record[iterator->first] = 0;
	}

	double summary = 0.0;
	double runtime = recordTime.back() - recordTime[0];

	for (int i = 1; i < recordEnergy.size(); i++)
	{
		for (auto iterator = record.begin(); iterator != record.end(); iterator++)
		{
			int cumulative = 0;
			record[iterator->first] += Tmode::getResultValue(recordEnergy[i - 1][iterator->first], recordEnergy[i][iterator->first], recordTime[i - 1] - recordTime[i], cumulative, iterator->first);

			if (cumulative > 0)
			{
				summary += record[iterator->first];
			}
		}
	}
	summary += POWER_BASELINE * runtime;

	if (verbose)
	{
		// MPI_Reduce (MAX, runtime, MPI_COMM_WORLD);
		std::cout << "Runtime [s] = " << runtime << std::endl;

		// MPI_Gather() //map -> vector
		for (std::map<std::string, unsigned long long int>::iterator iterator = recordEnergy[0].begin(); iterator != recordEnergy[0].end(); iterator++)
		{
			std::cout << Tmode::getCounterLabel(iterator->first) << " = " << record[iterator->first] << std::endl;
		}
		if (Tmode::getSummaryCounterName() != "")
		{
			std::cout << std::endl
					  << Tmode::getSummaryCounterName() << " = " << summary << std::endl;
		}
	}
	else
	{
		// MPI_Gather(0, double, summary, 1, COMM_WORLD);
		// TODO what if there is no summary
		std::cout << summary << std::endl;
	}
}

//----------------------------------------------------------------------------//
//----------------------------------------------------------------- MAIN -----//
//----------------------------------------------------------------------------//
template <typename Tmode>
void measureEnergy(bool verbose, double timeNow = 0.0)
{
	if (recordEnergy.empty())
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
	recordEnergy.push_back(energyNow);
	recordTime.push_back(timeNow);
}

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
			  << std::endl;
	exit(1);
}

bool isJobRunning(const std::string& jobName);
std::vector<std::string> listMatchingFiles(const std::string &prefix, const std::string &directory = ".");
double extractEnergyConsumption(const std::string &filepath);

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
		write(STDOUT_FILENO, "Interrupt signal (Ctrl+C) received\n", 36);

		if (child_pid > 0)
		{
			kill(child_pid, SIGINT); // Forward signal to child process
			write(STDOUT_FILENO, "Signal forwarded to the child process\n", 39);
		}
		else
		{
			write(STDOUT_FILENO, "Child not exist, terminating.\n", 31);
			isRunning = false;
			// _exit(0);
		}
	}
	else if (signal == SIGTERM)
	{
		write(STDOUT_FILENO, "Termination signal received.\n", 30);

		if (child_pid > 0)
		{
			kill(child_pid, SIGTERM); // Forward signal to child process
			write(STDOUT_FILENO, "Signal forwarded to the child process\n", 39);
		}
		else
		{
			write(STDOUT_FILENO, "Child not exist, terminating.\n", 31);
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
		// redirect the output to the file created.
		std::string filename = "mericStatic_" + std::to_string(job_id) + "_" + hostname + ".data";
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
				prints.push_back(printValues<RAPL>);
			}
			else if (token == "NVML")
			{
				measurements.push_back(&measureEnergy<NVML>);
				prints.push_back(printValues<NVML>);
			}
			else if (token == "A64FX")
			{
				measurements.push_back(&measureEnergy<A64FX>);
				prints.push_back(printValues<A64FX>);
			}
			else if (token == "HWMON")
			{
				measurements.push_back(&measureEnergy<HWMON>);
				prints.push_back(printValues<HWMON>);
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
					if (WIFEXITED(status))
					{
						// Check if the child process exited normally.
						if (WEXITSTATUS(status) != 0)
						{
							std::cerr << "Error: Child process terminated with an error (exit status: "
									  << WEXITSTATUS(status) << ")" << std::endl;
						}
						else
							std::cout << "Success: Child process terminated successfully.\n";
					}
					else if (WIFSIGNALED(status))
					{
						// If the child process terminated due to a signal.
						std::cerr << "Error: Child process terminated by signal "
								  << WTERMSIG(status) << std::endl;
					}
					else if (WIFSTOPPED(status))
					{
						// If the child process is stopped.
						std::cerr << "Error: Child process is stopped by signal "
								  << WSTOPSIG(status) << std::endl;
					}
					else
					{
						std::cerr << "Error: Unexpected child process termination state." << std::endl;
					}

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
			std::vector<std::string> files = listMatchingFiles(prefix);
			double totalEnergy = 0.0;
			for (const auto &filepath : files)
			{
				totalEnergy += extractEnergyConsumption(filepath);
				std::cout << filepath << "\n";
			}

			std::cout << "Total Energy consumption [J] = " << totalEnergy << "\n";
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

double extractEnergyConsumption(const std::string &filepath)
{
	std::ifstream file(filepath);
	std::string line;
	std::regex energyRegex(R"(Energy consumption \[J\] = ([0-9.]+))");
	std::smatch match;

	while (std::getline(file, line))
	{
		if (std::regex_search(line, match, energyRegex))
		{
			return std::stod(match[1]);
		}
	}
	return 0.0; // If not found
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
bool isJobRunning(const std::string& jobName)
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
	std::cout << result << "\n";
    return !result.empty();
}