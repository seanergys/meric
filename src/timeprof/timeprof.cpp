#include <unistd.h>
//#include <fstream>
//#include <iomanip>
#include <iostream>
#include <string.h>
#include <time.h>
#include <map>
#include <fstream>

#include "../../include/timeprof.h"

typedef struct RtnDesc
{
	std::string name;
	double lastStart;	//time
	double maxDuration;	//runtime
	double minDuration;	//runtime
} RTN_DESC;

std::map<std::string, RtnDesc> funcPROBEDList;


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

void TIMEPROF_evaluate(const char * fileName, uint timeThrashold)
{
	std::ofstream LogFile;
	LogFile.open(fileName);
	for (auto& funcRecord : funcPROBEDList)
	{
		if (funcRecord.second.minDuration > timeThrashold)
		{
			LogFile << funcRecord.first << std::endl;
//			dr_fprintf(STDERR, "[*] %d\t%d\t%s\n",funcRecord.second.minDuration/1000, funcRecord.second.maxDuration/1000, funcRecord.first.c_str());
		}
	}
	LogFile.close();
}


