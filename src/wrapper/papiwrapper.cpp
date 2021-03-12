
#ifdef HAVE_PAPI_H
#include <papi.h>
#endif

#ifdef HAVE_CILK_CILK_API_H
#include <cilk/cilk_api.h> 
#endif

#include "papiwrapper.h"
#include "../basis/utils.h"

using namespace meric;


unsigned long long int PAPI::COUNTERMAX = LLONG_MAX;

#ifdef HAVE_PAPI_H
std::map<std::string, unsigned long long int> PAPI::counters;
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
		std::cerr << "PAPI thread init error\n";
		exit(1);
	}
	
	initMap(counters);

	std::vector<int> eventList;

	for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
	{
		int eventCode;
		if (PAPI_event_name_to_code((char *)iterator->first.c_str(), &eventCode) != PAPI_OK)
		{
				std::cerr << "PAPI WARNING - event "<< iterator->first <<" not available\n"; 
		}

		if (PAPI_query_event(eventCode) == PAPI_OK)
		{
			eventList.push_back(eventCode);
		}
		else
		{
			std::cerr << "PAPI WARNING - event " << iterator->first << " not available\n";
			counters.erase(iterator->first);
		}
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


void PAPI::getCounterValues(std::map<std::string, unsigned long long int> & record) 
{
	if (PAPI_read(eventSet, countersValues) != PAPI_OK)
	{
		std::cerr << "PAPI error - can not read counters\n";
		exit(6);
	}
	
	int i = 0;
	for(std::map<std::string, unsigned long long int>::iterator iterator = counters.begin(); iterator != counters.end(); iterator++)
	{
		record[iterator->first] = countersValues[i];
		i++;
	}
}


#else
#endif

void PAPI::getRatios(std::map<std::string, unsigned long long int>& start, std::map<std::string, unsigned long long int>& stop, double runtime, std::map <std::string, double> & ratios)
{
	if (stop.count(PAPI_COUNTERS::COUNTER_AVX))
	{
		ratios["Performance [GFLOPs]"] = CountersRates::FLOPS(
			stop[PAPI_COUNTERS::COUNTER_AVX] - 
			start[PAPI_COUNTERS::COUNTER_AVX], 
			runtime
			);			
	}
	if (stop.count(PAPI_COUNTERS::COUNTER_CACHE_MISSES) && stop.count(PAPI_COUNTERS::COUNTER_HW_INSTRUCTIONS))
	{
		ratios["Computational Intensity"] = CountersRates::computationalIntensity(
				stop[PAPI_COUNTERS::COUNTER_HW_INSTRUCTIONS] - 
				start[PAPI_COUNTERS::COUNTER_HW_INSTRUCTIONS], 
				stop[PAPI_COUNTERS::COUNTER_CACHE_MISSES] - 
				start[PAPI_COUNTERS::COUNTER_CACHE_MISSES]
			);
	}

	if (stop.count(PAPI_COUNTERS::COUNTER_FREQ))
	{
		ratios["CPU frequency [Hz]"] = CountersRates::CPU_FREQUENCY_FROM_CYCLES(
				stop[PAPI_COUNTERS::COUNTER_FREQ] -
				start[PAPI_COUNTERS::COUNTER_FREQ],
				runtime
			);
	}
}

std::string PAPI::getSourceLabel()
{
	return "PAPI";
}

std::string PAPI::getCounterLabel(std::string counter)
{
	std::map<std::string, std::string>::iterator it = PAPI_COUNTERS::counterLabel.find(counter);
	if (it != PAPI_COUNTERS::counterLabel.end())
		return it->second;
	else
		return counter;
}

std::string PAPI::getSummaryCounterName()
{
	return "";
}

// solve counter overflow
unsigned long long PAPI::getResultValue(unsigned long long int startValue, unsigned long long int stopValue, double runtime, std::string counter, std::string regionName)
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

void PAPI::initMap(std::map<std::string, unsigned long long int> & dict)
{
	dict[PAPI_COUNTERS::COUNTER_AVX] = 0;
	dict[PAPI_COUNTERS::COUNTER_CACHE_MISSES] = 0;
	dict[PAPI_COUNTERS::COUNTER_CACHE_ACCESS] = 0;
	dict[PAPI_COUNTERS::COUNTER_HW_INSTRUCTIONS] = 0;
	dict[PAPI_COUNTERS::COUNTER_FREQ] = 0;
}



