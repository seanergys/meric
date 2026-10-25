#include "slave_script.h"

#include <sys/stat.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>

bool writeSlaveScript(const std::string &outPath, const std::string &masterIp)
{
	std::ofstream out(outPath, std::ios::out | std::ios::trunc);
	if (!out)
	{
		std::cerr << "Failed to open '" << outPath << "' for writing: " << std::strerror(errno) << "\n";
		return false;
	}

	out << "#!/bin/bash\n\n"
		   "# Configuration\n"
		   "MASTER_IP=\""
		<< masterIp
		<< "\"\n"
		   "PORT=5000\n\n"
		   "# 1. Send START signal\n"
		   "# Use 'nc' (Netcat) to send the string to the Master's IP/Port\n"
		   "echo -n \"START $(hostname -s)\" | nc -n $MASTER_IP $PORT\n"
		   "echo \"Sent START $(hostname -s) signal to Master.\"\n\n"
		   "# 2. Execute your program\n"
		   "echo \"Starting workload...\"\n"
		   "# --- REPLACE THE LINE BELOW WITH YOUR ACTUAL PROGRAM ---\n"
		   "sleep 5  # Example: simulates a program running for 5 seconds\n"
		   "# ./your_actual_program_here --args\n"
		   "# -------------------------------------------------------\n\n"
		   "# 3. Send STOP signal\n"
		   "echo -n \"STOP $(hostname -s)\" | nc -n $MASTER_IP $PORT\n"
		   "echo \"Workload finished. Sent STOP $(hostname -s) signal to Master.\"\n";

	out.close();
	if (!out)
	{
		std::cerr << "Failed while writing '" << outPath << "'.\n";
		return false;
	}

	if (chmod(outPath.c_str(), 0755) != 0)
	{
		std::cerr << "Warning: chmod(0755) failed for '" << outPath << "': " << std::strerror(errno) << "\n";
	}

	return true;
}
