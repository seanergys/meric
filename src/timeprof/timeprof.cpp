#include <unistd.h>
#include <iostream>
#include <string.h>
#include <time.h>
#include <map>
#include <fstream>

#include "../../include/timeprof.h"

typedef struct RtnDesc
{
	std::string name;
	double lastStart;	//timestamp
	double maxDuration;	//runtime
	double minDuration;	//runtime
} RTN_DESC;

std::map<std::string, RtnDesc> funcPROBEDList;

/**
 * starts time measurement for region 'regionName'
 **/
void TIMEPROF_regionStart(const char * regionName)
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

/**
 * stops time measurement for region 'regionName'
 **/
double TIMEPROF_regionStop(const char * regionName)
{
	double duration = 0.0;
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
	return duration;
}

/**
 * to the file 'fileName' stores a list of regions with a runtime longer than the trashold [ms]
 **/
void TIMEPROF_evaluate(const char * fileName, uint timeThrashold)
{
	double tim = timeThrashold/1000.0;
	std::ofstream LogFile;
	LogFile.open(fileName);
	
	for (auto& funcRecord : funcPROBEDList)
	{
		if (funcRecord.second.minDuration > tim)
		{
			LogFile << funcRecord.first << std::endl;
			//std::cout << funcRecord.first << " - " << funcRecord.second.minDuration << std::endl;
		}
	}
	LogFile.close();
}


