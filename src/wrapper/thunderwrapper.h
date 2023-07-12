
#ifndef SRC_WRAPPER_THUNDERWRAPPER_H_
#define SRC_WRAPPER_THUNDERWRAPPER_H_

#include "../basis/utils.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <ctime>
#include <array>
#include <vector>
#include <cmath>
#include <limits>

#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

namespace meric {

struct thunderSample {
	double timestamp;
	double DC;
//	double GPU;	//jetson only
//	double CPU;	//jetson only
};

class THUNDER {

public:
	static std::vector<std::string> vrSensorLabel; //"DC"
	static int FREQUENCY;
	static const int PROBE_NUMBER = 1;	//DC
	static const std::string SUM_ENERGY_LABEL;

	static void init();
	static int getSamples();
	static void getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> samples);
	static std::string getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double energy[], double minWatts[], double maxWatts[]);
	static void setTimeAdjustment(timespec time) {}; //needed for HDEEM
	static void fillTime(timespec &time) { clock_gettime(CLOCK_REALTIME, &time); };
	static void deleteSamples() {};
	static std::string getSourceLabel() {	return "THUNDER";};

private:
	static std::string filename;
	static std::vector<thunderSample> _samples;
};


}



#endif /* SRC_WRAPPER_THUNDERWRAPPER_H_ */
