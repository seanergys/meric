
#include "timeprof.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <time.h>


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

namespace timeprof{

enum TIMEPROF_RANKING {PREV_INIT=-1, POST_FINALIZE=-2, MASTER=0};
int TIMEPROF_rank = PREV_INIT;


typedef struct RtnDesc
{
	std::string name;
	double lastStart;	//timestamp
	double maxDuration;	//runtime
	double minDuration;	//runtime
} RTN_DESC;
std::map<std::string, RtnDesc> funcPROBEDList;
double lastDuration = 0.0;
struct outputStream {
	outputStream(std::streambuf * buf, std::ostream & os) : os(os), old_buf(os.rdbuf(buf)) {}
	~outputStream() { os.rdbuf(old_buf); }

	std::ostream& os;
	std::streambuf * old_buf;
};


/*
struct region {
	region(const char * regionName) : name(std::string(regionName)) {TIMEPROF_regionStart(regionName);}
	~region() { TIMEPROF_regionStop(name.c_str()); }
	std::string name;
};
*/

/**
 * starts time measurement for region 'regionName'
 **/
//void TIMEPROF_regionStart(const char * regionName)
void regionStart(const char * regionName)
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
			lastDuration = 0.0;
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
		return 0.0;	
	double duration = 0.0;
	printout << "P1 "<< TIMEPROF_rank <<"\t"<<regionName<<std::endl;
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
	lastDuration = duration;
	return duration;
}

/**
 * to the file 'fileName' stores a list of regions with a runtime longer than the trashold [ms]
 * if fileName not specified the output will go to the std::cerr
 **/
//void TIMEPROF_evaluate(unsigned int timeThreshold, const char * fileName)
void evaluate(unsigned int timeThreshold, const char * fileName)
{
	printout << "E1 "<< TIMEPROF_rank << std::endl;
	if (TIMEPROF_rank == MASTER)
	{
		#pragma omp master
		{
			double timeLimit = timeThreshold/1000.0;
			std::ofstream of;
			if (fileName != NULL && fileName[0] != '\0')
			{
				of.open(fileName);
			}
			outputStream raii(of.rdbuf(), std::cerr);
			if (!of.is_open())
				raii.~outputStream();
			
	
			for (auto& funcRecord : funcPROBEDList)
			{
				if (funcRecord.second.minDuration > timeLimit)
				{
					std::cerr << funcRecord.first << "\t"<< ((!timeThreshold) ? std::to_string(funcRecord.second.minDuration) : "") << std::endl;
					printout << funcRecord.first << " - " << funcRecord.second.minDuration << std::endl;
				}
			}
			printout << "E2 "<<TIMEPROF_rank<< std::endl;
		}	
	}
	TIMEPROF_rank = POST_FINALIZE;
}

double getLastRegionDuration()
{
	return lastDuration;
}

} //timeprof namespace

