#include <unistd.h>
#include <iostream>
#include <string.h>
#include <time.h>
#include <map>
#include <fstream>

#include "../../include/timeprof.h"

#ifdef HAVE_OMP_H
	#include <omp.h>
#endif

#ifdef HAVE_MPI_H
	#include <mpi.h>
#endif

#ifdef DEBUG
	#define printout std::cout	
#else
	#define printout if (false) std::cout	
#endif

typedef struct RtnDesc
{
	std::string name;
	double lastStart;	//timestamp
	double maxDuration;	//runtime
	double minDuration;	//runtime
} RTN_DESC;

std::map<std::string, RtnDesc> funcPROBEDList;
enum TIMEPROF_RANKING {PREV_INIT=-1, POST_FINALIZE=-2, MASTER=0};
int TIMEPROF_rank = PREV_INIT;

/**
 * starts time measurement for region 'regionName'
 **/
void TIMEPROF_regionStart(const char * regionName)
{
	if (regionName && !regionName[0])
		return;

	printout << "T1 "<< TIMEPROF_rank <<"\t"<<regionName<<std::endl;
	if (TIMEPROF_rank == PREV_INIT)
	{
#ifdef HAVE_MPI_H
		int flag;
		MPI_Initialized(&flag);
		if (flag)
			MPI_Comm_rank(MPI_COMM_WORLD, &TIMEPROF_rank);
#else
		TIMEPROF_rank = MASTER;
#endif
	}
		
	
	if (TIMEPROF_rank == MASTER)
	{
		#pragma omp master
		{
			struct timespec time;
			clock_gettime(CLOCK_REALTIME, &time);
			double timestamp = time.tv_sec + time.tv_nsec/1e9;

			std::string funcName = std::string(regionName);
			if (!funcPROBEDList.count(funcName))
			{
				RtnDesc function;
				function.name = funcName;
				function.maxDuration = 0.0;
				function.minDuration = 0.0;
				function.lastStart = timestamp;
				funcPROBEDList[funcName]=function;
			}
			else
				funcPROBEDList[funcName].lastStart = timestamp;
		}
	}
}

/**
 * stops time measurement for region 'regionName'
 **/
double TIMEPROF_regionStop(const char * regionName)
{
	if (regionName && !regionName[0])
		return 0.0;

	printout << "P1 "<< TIMEPROF_rank <<"\t"<<regionName<<std::endl;
	double duration = 0.0;
	if (TIMEPROF_rank == MASTER)
	{
		#pragma omp master
		{
			std::string funcName = std::string(regionName);
			if (funcPROBEDList.count(funcName) && funcPROBEDList[funcName].lastStart != 0)
			{
				struct timespec time;
				clock_gettime(CLOCK_REALTIME, &time);
				duration = time.tv_sec + time.tv_nsec/1e9 - funcPROBEDList[funcName].lastStart;
	
				if (duration > (funcPROBEDList[funcName].maxDuration))
					funcPROBEDList[funcName].maxDuration = duration;
				if (duration < funcPROBEDList[funcName].minDuration || funcPROBEDList[funcName].minDuration == 0)
					funcPROBEDList[funcName].minDuration = duration;
				funcPROBEDList[funcName].lastStart=0;
			}
		}
	}
	return duration;
}

/**
 * to the file 'fileName' stores a list of regions with a runtime longer than the trashold [ms]
 **/
void TIMEPROF_evaluate(const char * fileName, uint timeThrashold)
{
	printout << "E1 "<< TIMEPROF_rank << std::endl;
	if (TIMEPROF_rank == MASTER)
	{
		#pragma omp master
		{
			double timeLimit = timeThrashold/1000.0;
			std::ofstream LogFile;
			LogFile.open(fileName);
	
			for (auto& funcRecord : funcPROBEDList)
			{
				if (funcRecord.second.minDuration > timeLimit)
				{
					LogFile << funcRecord.first << std::endl;
					printout << funcRecord.first << " - " << funcRecord.second.minDuration << std::endl;
				}
			}
			LogFile.close();
			printout << "E2 "<<TIMEPROF_rank<< std::endl;
		}	
	}
	TIMEPROF_rank = POST_FINALIZE;
}


