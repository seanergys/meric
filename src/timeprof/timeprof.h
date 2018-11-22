#ifndef SRC_TIMEPROF_TIMEPROF_H_
#define SRC_TIMEPROF_TIMEPROF_H_

namespace timeprof{
void regionStart(const char * regionName);
double regionStop(const char * regionName);
void evaluate(unsigned int timeThrashold, const char * fileName);
double getLastRegionDuration();
}

#endif //SRC_TIMEPROF_TIMEPROF_H_
