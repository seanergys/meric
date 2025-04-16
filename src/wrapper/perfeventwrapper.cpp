// #define _GNU_SOURCE
#ifdef HAVE_LINUX_PERF_EVENT_H
#include <linux/perf_event.h>
#endif

#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <limits.h>
// #include <errno.h>

#include "perfeventwrapper.h"
#include "../basis/utils.h"

using namespace meric;

bool PERFEVENT::detailedMode = false;
unsigned long long int PERFEVENT::COUNTERMAX = LLONG_MAX;

#ifdef HAVE_LINUX_PERF_EVENT_H
std::map<const std::string, long> PERFEVENT::countersFd;

void PERFEVENT::init(bool detailed)
{
	detailedMode = detailed;

	auto setCounter = [&] (__u32 type, __u64 config, int cpu)
	{
		struct perf_event_attr attr;
		memset(&attr, 0, sizeof(struct perf_event_attr));
		attr.size = sizeof(struct perf_event_attr);
		attr.inherit = 1;
		attr.type = type;
		attr.config = config;
		attr.disabled = 1;

		return openCounter(attr, cpu);
	};
	
	//Initialization of per-core monitoring of CPU cycles and CPU instructions
	for(int i = 0; i < Environment::CORES_PER_SOCKET*Environment::SOCKETS_PER_NODE; i++)
	{
		std::string suffix = "_" + std::to_string(i);
		
		countersFd[PERFEVENT_COUNTERS::PERF_CPU_CYCLES + suffix] = setCounter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, i);
		countersFd[PERFEVENT_COUNTERS::PERF_CPU_INSTS + suffix] = setCounter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, i);
	}

	#ifdef _INTEL_
	//Initialization of per-core monitoring of supported Intel counters
	for(int i = 0; i < Environment::CORES_PER_SOCKET*Environment::SOCKETS_PER_NODE; i++)
	{
		std::string suffix = "_" + std::to_string(i);

		//countersFd[PERFEVENT_COUNTERS::INTEL_CYCLES_AMX_ACTIVE + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_CYCLES_AMX_ACTIVE], i);
		countersFd[PERFEVENT_COUNTERS::INTEL_FP_INSTS_SCALAR + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_FP_INSTS_SCALAR], i);
		countersFd[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC128_S + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC128_S], i);
		countersFd[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC128_D + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC128_D], i);
		countersFd[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC256_S + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC256_S], i);
		countersFd[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC256_D + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC256_D], i);
		countersFd[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC512_S + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC512_S], i);
		countersFd[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC512_D + suffix] = setCounter(PERF_TYPE_RAW, PERFEVENT_COUNTERS::counterDefines[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC512_D], i);
	}
	#endif

	start();
}

void PERFEVENT::close()
{
	for(std::map<const std::string, long>::iterator iterator = countersFd.begin(); iterator != countersFd.end(); iterator++)
	{
		if (ioctl(iterator->second, PERF_EVENT_IOC_DISABLE, 0) != 0)
		{
			std::cerr << "PERFEVENT error when stopping counting\n";
		}
	}
	
	for(std::map<const std::string, long>::iterator iterator = countersFd.begin(); iterator != countersFd.end(); iterator++)
	{
		if (::close(iterator->second) != 0)
		{
			std::cerr << "PERFEVENT error when closing file descriptor\n";
		}
	}
}

__u32 PERFEVENT::getPMUtype(std::string PMUname)
{
	std::string pathToPMUtypeFile = "/sys/bus/event_source/devices/"+PMUname+"/type";

	__u32 PMUtype;

	std::ifstream PMUtypeFile;
	PMUtypeFile.open(pathToPMUtypeFile);
	if(PMUtypeFile.is_open())
	{
		std::string PMUtypeStr;
		if(!std::getline(PMUtypeFile, PMUtypeStr).fail())
		{
			PMUtype = (__u32)std::stoul(PMUtypeStr);
		}
		else
		{
			std::cerr << "PERFEVENT error while reading file " << pathToPMUtypeFile << "\n";			
			PMUtype = 0;
		}

		PMUtypeFile.close();
	}
	else
	{
		std::cerr << "PERFEVENT error while opening file " << pathToPMUtypeFile << "\n";
		PMUtype = 0;
	}

	return PMUtype;
}

long PERFEVENT::openCounter(perf_event_attr attr, int cpu)
{
	//(struct perf_event_attr, pid, cpu, group_fd, flags);
	long fd = syscall(__NR_perf_event_open, &attr, -1, cpu, -1, 0);
/*
pid == 0 and cpu == -1
    This measures the calling process/thread on any CPU. 
pid == 0 and cpu >= 0
    This measures the calling process/thread only when running on the specified CPU. 
pid > 0 and cpu == -1
    This measures the specified process/thread on any CPU. 
pid > 0 and cpu >= 0
    This measures the specified process/thread only when running on the specified CPU. 
pid == -1 and cpu >= 0
    This measures all processes/threads on the specified CPU. This requires CAP_SYS_ADMIN capability or a /proc/sys/kernel/perf_event_paranoid value of less than 1. 
pid == -1 and cpu == -1
    This setting is invalid and will return an error.
*/
	if (fd < 0)
	{
		//int err = errno;
		std::cerr << "PERFEVENT error while opening new fd\n";
	}
	
	return fd;
}

void PERFEVENT::start()
{
	for(std::map<const std::string, long>::iterator iterator = countersFd.begin(); iterator != countersFd.end(); iterator++)
	{
		if (ioctl(iterator->second, PERF_EVENT_IOC_RESET, 0) != 0)
		{
			std::cerr << "PERFEVENT error when resetting counter\n";
		}
		if (ioctl(iterator->second, PERF_EVENT_IOC_ENABLE, 0) != 0)
		{
			std::cerr << "PERFEVENT error when starting counting\n";
		}
	}
}

void PERFEVENT::getCounterValues(std::map<std::string, unsigned long long int> & record)
{
	for(std::map<const std::string, long>::iterator iterator = countersFd.begin(); iterator != countersFd.end(); iterator++)
	{
		unsigned long long int result = 0;
		if ((read(iterator->second, &result, sizeof(unsigned long long int))) != sizeof(unsigned long long int))
		{
			//int errn = errno;
			std::cerr << "PERFEVENT error. Failed to read counter "<< iterator->first << std::endl;
			record[iterator->first] = 0;
		}
		else
		{
			record[iterator->first] = result;
		}
	}
}

#else
#endif

void PERFEVENT::getRatios(std::map<std::string, unsigned long long int>& start, std::map<std::string, unsigned long long int>& stop, double runtime, std::map <std::string, double> & ratios)
{
	int cumulative = 0;

	for(int i = 0; i < Environment::CORES_PER_SOCKET*Environment::SOCKETS_PER_NODE; i++)
	{
		std::string suffix = "_" + std::to_string(i);

		unsigned long long int cycles = 0;
		if(stop.count(PERFEVENT_COUNTERS::PERF_CPU_CYCLES + suffix))
		{
			cycles = getResultValue(start[PERFEVENT_COUNTERS::PERF_CPU_CYCLES + suffix],
				stop[PERFEVENT_COUNTERS::PERF_CPU_CYCLES + suffix], runtime, cumulative);
		}

		ratios["CPU_CORE_FREQUENCY"+suffix+" [Hz]"] = CountersRates::CPU_FREQUENCY_FROM_CYCLES(cycles, runtime);

		unsigned long long int instructions = 0;
		if(stop.count(PERFEVENT_COUNTERS::PERF_CPU_INSTS + suffix))
		{
			instructions = getResultValue(start[PERFEVENT_COUNTERS::PERF_CPU_INSTS + suffix],
				stop[PERFEVENT_COUNTERS::PERF_CPU_INSTS + suffix], runtime, cumulative);
		}

		#ifdef _INTEL_
		//unsigned long long int cyclesAMXActive = 0;
		//if(stop.count(PERFEVENT_COUNTERS::INTEL_CYCLES_AMX_ACTIVE + suffix))
		//{
		//	cyclesAMXActive = getResultValue(start[PERFEVENT_COUNTERS::INTEL_CYCLES_AMX_ACTIVE + suffix],
		//		stop[PERFEVENT_COUNTERS::INTEL_CYCLES_AMX_ACTIVE + suffix], runtime, cumulative);
		//}
		
		//ratios["CPU_AMX_ACTIVE"+suffix+" [%]"] = cyclesAMXActive/cycles;

		unsigned long long int instructionsScalarFp = 0;
		if(stop.count(PERFEVENT_COUNTERS::INTEL_FP_INSTS_SCALAR + suffix))
		{
			instructionsScalarFp = getResultValue(start[PERFEVENT_COUNTERS::INTEL_FP_INSTS_SCALAR + suffix],
				stop[PERFEVENT_COUNTERS::INTEL_FP_INSTS_SCALAR + suffix], runtime, cumulative);
		}

		unsigned long long int instructionsVec128FpSP = 0;
		if(stop.count(PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC128_S + suffix))
		{
			instructionsVec128FpSP = getResultValue(start[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC128_S + suffix],
				stop[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC128_S + suffix], runtime, cumulative);
		}

		unsigned long long int instructionsVec128FpDP = 0;
		if(stop.count(PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC128_D + suffix))
		{
			instructionsVec128FpDP = getResultValue(start[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC128_D + suffix],
				stop[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC128_D + suffix], runtime, cumulative);
		}

		unsigned long long int instructionsVec256FpSP = 0;
		if(stop.count(PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC256_S + suffix))
		{
			instructionsVec256FpSP = getResultValue(start[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC256_S + suffix],
				stop[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC256_S + suffix], runtime, cumulative);
		}

		unsigned long long int instructionsVec256FpDP = 0;
		if(stop.count(PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC256_D + suffix))
		{
			instructionsVec256FpDP = getResultValue(start[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC256_D + suffix],
				stop[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC256_D + suffix], runtime, cumulative);
		}

		unsigned long long int instructionsVec512FpSP = 0;
		if(stop.count(PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC512_S + suffix))
		{
			instructionsVec512FpSP = getResultValue(start[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC512_S + suffix],
				stop[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC512_S + suffix], runtime, cumulative);
		}

		unsigned long long int instructionsVec512FpDP = 0;
		if(stop.count(PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC512_D + suffix))
		{
			instructionsVec512FpDP = getResultValue(start[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC512_D + suffix],
				stop[PERFEVENT_COUNTERS::INTEL_FP_INSTS_VEC512_D + suffix], runtime, cumulative);
		}
		
		unsigned long long int instructionsVec128Fp = instructionsVec128FpSP + instructionsVec128FpDP;
		unsigned long long int instructionsVec256Fp = instructionsVec256FpSP + instructionsVec256FpDP;
		unsigned long long int instructionsVec512Fp = instructionsVec512FpSP + instructionsVec512FpDP;
		unsigned long long int instructionsVecFp = instructionsVec128Fp + instructionsVec256Fp + instructionsVec512Fp;
		unsigned long long int instructionsFp = instructionsScalarFp + instructionsVecFp;

		ratios["CPU_INSTRUCTION SCALAR_FP/FP"+suffix+" [%]"] = instructionsFp ? instructionsScalarFp/(double)instructionsFp : 0;
		ratios["CPU_INSTRUCTION VEC_128B_FP/FP"+suffix+" [%]"] = instructionsFp ? instructionsVec128Fp/(double)instructionsFp : 0;
		ratios["CPU_INSTRUCTION VEC_256B_FP/FP"+suffix+" [%]"] = instructionsFp ? instructionsVec256Fp/(double)instructionsFp : 0;
		ratios["CPU_INSTRUCTION VEC_512B_FP/FP"+suffix+" [%]"] = instructionsFp ? instructionsVec512Fp/(double)instructionsFp : 0;
		ratios["CPU_INSTRUCTION VEC_FP/FP"+suffix+" [%]"] = instructionsFp ? instructionsVecFp/(double)instructionsFp : 0;

		ratios["CPU_INSTRUCTION SCALAR_FP/ALL"+suffix+" [%]"] = instructions ? instructionsScalarFp/(double)instructions : 0;
		ratios["CPU_INSTRUCTION VEC_128B_FP/ALL"+suffix+" [%]"] = instructions ? instructionsVec128Fp/(double)instructions : 0;
		ratios["CPU_INSTRUCTION VEC_256B_FP/ALL"+suffix+" [%]"] = instructions ? instructionsVec256Fp/(double)instructions : 0;
		ratios["CPU_INSTRUCTION VEC_512B_FP/ALL"+suffix+" [%]"] = instructions ? instructionsVec512Fp/(double)instructions : 0;
		ratios["CPU_INSTRUCTION VEC_FP/ALL"+suffix+" [%]"] = instructions ? instructionsVecFp/(double)instructions : 0;

		unsigned long long int flops = instructionsScalarFp + 4*instructionsVec128FpSP + 2*instructionsVec128FpDP +
								8*instructionsVec256FpSP + 4*instructionsVec256FpDP + 16*instructionsVec512FpSP + 8*instructionsVec512FpDP;

		ratios["CPU_FLOP"+suffix+" [FLOP/s]"] = flops/runtime;
		#endif
	}
}

std::string PERFEVENT::getSourceLabel()
{
	return "PERFEVENT";
}

std::string PERFEVENT::getCounterLabel(std::string counter)
{
	if (!detailedMode)
	{
		return "";
	}
	
	size_t suffixStartPos = counter.find_last_of('_');
	size_t suffixLength = counter.size() - suffixStartPos;
	std::string counterBaseName = counter.substr(0, suffixStartPos);

	std::map<const std::string, std::string>::iterator iterator = PERFEVENT_COUNTERS::counterLabel.find(counterBaseName);
	if (iterator != PERFEVENT_COUNTERS::counterLabel.end())
	{
		return iterator->second + counter.substr(suffixStartPos, suffixLength);
	}
	else
	{
		return counter;
	}
}

std::string PERFEVENT::getSummaryCounterName()
{
	return "";
}

// solve counter overflow
unsigned long long PERFEVENT::getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, int & cumulative, std::string counter, std::string regionName)
{
	cumulative = 0;
	if (stopValue < startValue)
	{
		MERIC_INFO << "PERFEVENT OVERFLOW, MAX VALUE: " << COUNTERMAX <<" " <<counter << std::endl;
		MERIC_INFO << "PERFEVENT REPAIR " << startValue << " .. " << stopValue << " = " << COUNTERMAX- startValue + stopValue << std::endl;
		return COUNTERMAX - startValue + stopValue;
	}
	else
	{
		return stopValue - startValue;
	}
}

void PERFEVENT::initMap(std::map<std::string, unsigned long long int> & dict)
{
	for (int i=0; i < Environment::SOCKETS_PER_NODE*Environment::CORES_PER_SOCKET; i++)
	{
		std::string suffix = "_" + std::to_string(i);

		dict[PERFEVENT_COUNTERS::PERF_CPU_CYCLES + suffix] = 0;
	}
}
