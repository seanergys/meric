
#ifdef HAVE_PAPI_H
#include <papi.h>
#endif

#ifdef HAVE_CILK_CILK_API_H
#include <cilk/cilk_api.h> 
#endif

#include <vector>
#include <limits.h>
#include <omp.h>

#include "papiwrapper.h"
#include "counters.h"
#include "../basis/utils.h"

using namespace meric;


long long PAPI::COUNTERMAX = LLONG_MAX;

#ifdef HAVE_PAPI_H
std::map<std::string, long long> PAPI::counters;
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

	int ret;
/*
#ifdef HAVE_CILK_CILK_API_H
	if (__cilkrts_get_nworkers() > omp_get_thread_num())
		ret = PAPI_thread_init((unsigned long (*)(void))(__cilkrts_get_nworkers));
	else
#elif HAVE_OMP_H
*/
#ifdef HAVE_OMP_H
	ret = PAPI_thread_init((unsigned long (*)(void))(omp_get_thread_num));
#else
	ret = PAPI_OK;
#endif		
	if (ret != PAPI_OK)
	{
		printf("PAPI thread init error\n");
		exit(1);
	}
	
	initMap(counters);

	std::vector<int> eventList;

	for(std::map<std::string, long long>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
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


void PAPI::getValues(std::map<std::string, long long> & record) 
{
	if (PAPI_read(eventSet, countersValues) != PAPI_OK)
	{
		std::cerr << "PAPI error - can not read counters\n";
		exit(6);
	}
	
	int i = 0;
	for(std::map<std::string, long long>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
	{
		record[iterator->first] = countersValues[i];
		i++;
	}
}


#else
#endif

// solve counter overflow
unsigned long long PAPI::getResultValue(long long startValue, long long stopValue, std::string counter)
{
	if (stopValue < startValue)
	{
		MERIC_INFO << "PAPI OVERFLOW, MAX VALUE: " << COUNTERMAX <<" " <<counter << std::endl;
		MERIC_INFO << "PAPI REPAIR " << startValue << " .. " << stopValue << " = " << COUNTERMAX- startValue + stopValue << std::endl;
		return COUNTERMAX - startValue + stopValue;
	}
	else
		return stopValue - startValue;
}

void PAPI::initMap(std::map<std::string, long long> & dict)
{
	dict[PAPI_COUNTERS::COUNTER_AVX] = 0;
	dict[PAPI_COUNTERS::COUNTER_CACHE_MISSES] = 0;
	dict[PAPI_COUNTERS::COUNTER_CACHE_ACCESS] = 0;
	dict[PAPI_COUNTERS::COUNTER_HW_INSTRUCTIONS] = 0;
}



