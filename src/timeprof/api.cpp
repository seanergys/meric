
#include "../../include/timeprof.h"
#include "timeprof.h"

void   TIMEPROF_regionStart (const char * regionName)
{
	timeprof::regionStart(regionName);
}

double TIMEPROF_regionStop  (const char * regionName)
{
	return timeprof::regionStop(regionName);
}

void   TIMEPROF_evaluate    (unsigned int timeThreshold, const char * fileName)
{
	timeprof::evaluate(timeThreshold, fileName);
}

double TIMEPROF_getLastRegionDuration()
{
	return timeprof::getLastRegionDuration();
}
