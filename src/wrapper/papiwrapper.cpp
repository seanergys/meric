/**
 * To add new PAPI counter, add counter Name to papiwrapper.h
 * 
 **/
/******************************************************************************/
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <ctime>
#include <array>
#include <unistd.h>
/******************************************************************************/
#include <papi.h>
#include <sys/types.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <asm/unistd.h>
#include <errno.h>
/******************************************************************************/
#include <map>
#include <vector>

#include "papiwrapper.h"
#include "counters.h"

using namespace meric;

#ifndef USE_DUMMY
std::map<std::string, long> PAPI::counters;
int PAPI::eventSet = PAPI_NULL;
long long * PAPI::countersValues = NULL;

void PAPI::init()
{
	if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
	{
		std::cerr << "PAPI library init error!\n";
		exit(6);
	}

	if(PAPI_create_eventset(&eventSet) != PAPI_OK) 
	{
		std::cerr << "PAPI error - can not create event set\n";
		exit(6);
	}
	
	initMap(counters);

	std::vector<int> eventList;

	for(std::map<std::string, long>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
	{
		int eventCode;
		if (PAPI_event_name_to_code((char *)iterator->first.c_str(), &eventCode) != PAPI_OK)
		{
				std::cerr << "PAPI error - requested event "<< iterator->first <<" not available\n"; 
				exit(6);
		}
		eventList.push_back(eventCode);
	}
	
	if (PAPI_add_events(eventSet, &eventList[0], eventList.size()) != PAPI_OK)
	{
		std::cerr << "PAPI error - can not add events to watch\n";
		exit(6);
	}
	
	if (PAPI_start(eventSet) != PAPI_OK)
	{
		std::cerr << "PAPI error - can not start counting\n";
		exit(6);
	}
	
	countersValues = (long long *) malloc (sizeof(long long) * counters.size());
	if (countersValues == NULL)
	{
		std::cerr << "MALLOC error\n";
		exit(1);
	}
}


void PAPI::close()
{
	free(countersValues);
	
	if (PAPI_stop(eventSet, NULL) != PAPI_OK)
	{
		std::cerr << "PAPI error - can not stop counting\n";
		exit(6);
	}
	
	PAPI_shutdown();
}


void PAPI::getValues(std::map<std::string, long> & record) 
{
	if (PAPI_read(eventSet, countersValues) != PAPI_OK)
	{
		std::cerr << "PAPI error - can not read counters\n";
		exit(6);
	}
	
	int i = 0;
	for(std::map<std::string, long>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
	{
		record[iterator->first] = countersValues[i];
		i++;
	}
}


#else
#endif

void PAPI::initMap(std::map<std::string, long> & dict)
{
	dict[PAPI_COUNTERS::COUNTER_AVX] = 0;
	dict[PAPI_COUNTERS::COUNTER_CACHE_MISSES] = 0;
	dict[PAPI_COUNTERS::COUNTER_CACHE_ACCESS] = 0;
	dict[PAPI_COUNTERS::COUNTER_HW_INSTRUCTIONS] = 0;
}



