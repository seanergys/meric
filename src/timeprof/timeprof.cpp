#include "timeprof.h"
#include "../basis/utils.h"

#include <iostream>
#include <fstream>
#include <map>
#include <time.h>
#include <string.h>

#ifdef HAVE_OMP_H
	#include <omp.h>
#endif

#ifdef HAVE_MPI_H
	#include <mpi.h>
#endif

namespace timeprof{

enum TIMEPROF_RANKING {PREV_INIT=-1, POST_FINALIZE=-2, MASTER=0};
int TIMEPROF_rank = PREV_INIT;
bool TIMEPROF_master = true;


typedef struct RtnDesc
{
	double lastStart;   //timestamp
	double maxDuration; //runtime
	double minDuration; //runtime
	int    startCNT;    //number of starts
	int    stopCNT;     //number of stops
} RTN_DESC;
static std::map<std::string, RtnDesc> funcPROBEDList;
double lastDuration = 0.0;
struct outputStream {
	outputStream(std::streambuf * buf, std::ostream & os) : os(os), old_buf(os.rdbuf(buf)) {}
	~outputStream() { os.rdbuf(old_buf); }

	std::ostream& os;
	std::streambuf * old_buf;
};


/**
 * starts time measurement for region 'regionName'
 **/
//void TIMEPROF_regionStart(const char * regionName)
void regionStart(const char * regionName)
{
	if (regionName && !regionName[0])
	{
		return;
	}

	if (TIMEPROF_rank == PREV_INIT)
	{
		TIMEPROF_master = meric::mutils::getenv<bool>("TIMEPROF_MASTER", true);
#ifdef HAVE_MPI_H
		int flag;
		MPI_Initialized(&flag);
		if (flag)
		{
			MPI_Comm_rank(MPI_COMM_WORLD, &TIMEPROF_rank);
		}
#else
		TIMEPROF_rank = MASTER;
#endif
	}
		
	
	if (TIMEPROF_master == false || TIMEPROF_rank == MASTER)
	{
		#pragma omp master
		{
			struct timespec time;
			clock_gettime(CLOCK_REALTIME, &time);
			double timestamp = time.tv_sec + time.tv_nsec/1e9;
			
			std::string funcName = std::string(regionName);
			if (!funcPROBEDList.count(funcName))
			{
				RtnDesc function = {
					.lastStart   = timestamp,
					.maxDuration = 0.0,
					.minDuration = 0.0,
					.startCNT    = 1,
					.stopCNT     = 0
					};
				funcPROBEDList[funcName] = function;
			}
			else
			{
				funcPROBEDList[funcName].startCNT++;
				funcPROBEDList[funcName].lastStart = timestamp;
			}
			lastDuration = 0.0;
			
			MERIC_INFO << "[" + std::to_string(TIMEPROF_rank) + "] START " + std::string(regionName) + " CNT " +std::to_string(funcPROBEDList[funcName].startCNT)+" / "+std::to_string(funcPROBEDList[funcName].stopCNT)  +"\n";
		}
	}
}

/**
 * stops time measurement for region 'regionName'
 **/
//double TIMEPROF_regionStop(const char * regionName)
double regionStop(const char * regionName)
{
	if (regionName && !regionName[0])
	{
		return 0.0;
	}
	double duration = 0.0;
	
	if (TIMEPROF_master == false || TIMEPROF_rank == MASTER)
	{
		#pragma omp master
		{
			std::string funcName = std::string(regionName);
			if (funcPROBEDList.count(funcName))
			{
				funcPROBEDList[funcName].stopCNT++;
				
				MERIC_INFO << "[" + std::to_string(TIMEPROF_rank) + "] STOP " + funcName + " CNT " +std::to_string(funcPROBEDList[funcName].startCNT)+" / "+std::to_string(funcPROBEDList[funcName].stopCNT)  +"\n";
				
				if (funcPROBEDList[funcName].lastStart != 0)
				{
					struct timespec time;
					clock_gettime(CLOCK_REALTIME, &time);
					duration = meric::mutils::timeToDouble(time) - funcPROBEDList[funcName].lastStart;
					
					if (duration > (funcPROBEDList[funcName].maxDuration))
					{
						funcPROBEDList[funcName].maxDuration = duration;
					}
					if (duration < funcPROBEDList[funcName].minDuration || funcPROBEDList[funcName].minDuration == 0)
					{
						funcPROBEDList[funcName].minDuration = duration;
					}
					funcPROBEDList[funcName].lastStart = 0;
				}
			}
			else
			{
				MERIC_INFO << "[" + std::to_string(TIMEPROF_rank) + "] warning: STOP " + funcName + " prior START\n";
			}
		}
	}
	lastDuration = duration;
	return duration;
}

/**
 * to the file 'fileName' stores a list of regions with a runtime longer than the threshold [ms]
 * if fileName not specified the output will go to the std::cerr
 **/
//void TIMEPROF_evaluate()
void evaluate()
{
	double timeLimit = meric::mutils::getenv<int>("TIMEPROF_TIME", 0) / 1e3; //milliseconds to seconds
	std::string fileName = meric::mutils::getenv("TIMEPROF_OUTPUT", std::string("TIMEPROF.log"));
	std::string output = "";

#ifdef HAVE_MPI_H
	if (TIMEPROF_master == false)
	{
		#pragma omp master
		{
			unsigned int loopSize = funcPROBEDList.size();
			MPI_Bcast(&loopSize, 1, MPI_UNSIGNED, MASTER, MPI_COMM_WORLD);
			
			if (TIMEPROF_rank == MASTER)
			{
				for (auto& funcRecord : funcPROBEDList)
				{
					//master broadcasts region name
					//processes reply boolean
						// return (region entered == true && startCNT == stopCNT && region min duration > timeLimit)
					//if all regions returned true
						// MPI_Reduce minDuration
						// MPI_Gather startCNT
					// if every process has the same startCNT
						//print add the process to the output
					
					int stringSize = funcRecord.first.length()+1;
					MPI_Bcast(&stringSize, 1, MPI_UNSIGNED, MASTER, MPI_COMM_WORLD);
					char * regionName = new char [stringSize];
					strcpy(regionName, funcRecord.first.c_str());
					MPI_Bcast(regionName, stringSize, MPI_CHAR, MASTER, MPI_COMM_WORLD);
					delete[] regionName;
					bool avail = true;
					MPI_Allreduce(MPI_IN_PLACE, &avail, 1, MPI_C_BOOL, MPI_LAND, MPI_COMM_WORLD);
					
					if (avail)
					{
						double duration;
						int min, max;
						MPI_Reduce(&funcRecord.second.minDuration, &duration, 1, MPI_DOUBLE, MPI_MIN, MASTER, MPI_COMM_WORLD);
						MPI_Reduce(&funcRecord.second.startCNT, &min, 1, MPI_INT, MPI_MIN, MASTER, MPI_COMM_WORLD);
						MPI_Reduce(&funcRecord.second.startCNT, &max, 1, MPI_INT, MPI_MAX, MASTER, MPI_COMM_WORLD);
						if (min == max)
						{
							output += funcRecord.first + "\t" + ((!timeLimit) ? std::to_string(duration) : "") + "\n";
						}
					}
				}
			}
			else
			{
				for (int i = 0; i<loopSize; i++)
				{
					int stringSize;
					MPI_Bcast(&stringSize, 1, MPI_UNSIGNED, MASTER, MPI_COMM_WORLD);
					char * regionName = new char [stringSize];
					MPI_Bcast(regionName, stringSize, MPI_CHAR, MASTER, MPI_COMM_WORLD);
					
					bool avail = (funcPROBEDList.count(regionName) && funcPROBEDList[regionName].startCNT == funcPROBEDList[regionName].stopCNT && funcPROBEDList[regionName].minDuration > timeLimit);
					MPI_Allreduce(MPI_IN_PLACE, &avail, 1, MPI_C_BOOL, MPI_LAND, MPI_COMM_WORLD);
					if (avail)
					{
						double duration;
						int min, max;
						MPI_Reduce(&funcPROBEDList[regionName].minDuration, &duration, 1, MPI_DOUBLE, MPI_MIN, MASTER, MPI_COMM_WORLD);
						MPI_Reduce(&funcPROBEDList[regionName].startCNT, &min, 1, MPI_INT, MPI_MIN, MASTER, MPI_COMM_WORLD);
						MPI_Reduce(&funcPROBEDList[regionName].startCNT, &max, 1, MPI_INT, MPI_MAX, MASTER, MPI_COMM_WORLD);
					}
					delete[] regionName;
				}
			}
		}
	}
	else
#endif
	{
		if (TIMEPROF_rank == MASTER)
		{
			#pragma omp master
			{
				for (auto& funcRecord : funcPROBEDList)
				{
					if (funcRecord.second.minDuration > timeLimit && funcRecord.second.startCNT == funcRecord.second.stopCNT)
					{
						output += funcRecord.first + "\t" + ((!timeLimit) ? std::to_string(funcRecord.second.minDuration) : "") + "\n";
					}
				}
			}
		}
	}
	
	#pragma omp master
	if (TIMEPROF_rank == MASTER)
	{
		std::ofstream of;
		if (fileName != "")
		{
			of.open(fileName);
		}
		outputStream raii(of.rdbuf(), std::cerr);
		if (!of.is_open())
		{
			raii.~outputStream();
		}
		
		std::cerr << output;
	}
	TIMEPROF_rank = POST_FINALIZE;
}

double getLastRegionDuration()
{
	return lastDuration;
}

} //timeprof namespace

