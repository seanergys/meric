#ifndef TIMEPROF_H_
#define TIMEPROF_H_

#ifdef __cplusplus
extern "C" {
#endif

void   TIMEPROF_regionStart (const char * regionName);
double TIMEPROF_regionStop  (const char * regionName);
void   TIMEPROF_evaluate    (const char * fileName, uint timeThrashold);

#ifdef __cplusplus
}
#endif

#endif // TIMEPROF_H_
