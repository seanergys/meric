#ifndef TIMEPROF_H_
#define TIMEPROF_H_

#ifdef __cplusplus
extern "C" {
#endif

void   TIMEPROF_regionStart (const char * regionName);
double TIMEPROF_regionStop  (const char * regionName);
void   TIMEPROF_evaluate    (unsigned int timeThrashold = 0, const char * fileName = "");
double TIMEPROF_getLastRegionDuration();

//void TIMEPROF_captureScope(const char * regionName);
//RAII scope measurement - it does not return time, can be read using TIMEPROF_getLastRegionDuration()
void TIMEPROF_scopeEnd(const char ** regionName)
{
    TIMEPROF_regionStop  (*regionName);
}
#define TIMEPROF_captureScope( regionName ) \
	TIMEPROF_regionStart (regionName); \
	const char * tmp __attribute__((cleanup(TIMEPROF_scopeEnd))) = regionName;

#ifdef __cplusplus
}
#endif

#endif // TIMEPROF_H_
