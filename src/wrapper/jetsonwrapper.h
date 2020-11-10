
#ifndef SRC_WRAPPER_JETSONWRAPPER_H_
#define SRC_WRAPPER_JETSONWRAPPER_H_

#include "../basis/utils.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <time.h>
#include <ctime>
#include <array>

#include <unistd.h>
#include <sys/stat.h>
#include <vector>
#include <cmath>



namespace meric {

struct jetsonSample {
	double timestamp;
	double DC;
	double GPU;
	double CPU;
};

class JETSON {

public:

	static std::vector<std::string> vrSensorLabel;// = {"GPU", "CPU"}; //"DC",
	static int FREQUENCY;
	static const int PROBE_NUMBER = 3;	//DC + GPU + CPU	//	static const int PROBE_NUMBER = 7; //TODO
	static const std::string SUM_ENERGY_LABEL;

	static void init();
	static int getSamples();
	static void getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> samples);
	static std::string getData(double startTime, double duration, size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR, double energy[], double minWatts[], double maxWatts[]);
	static void setTimeAdjustment(timespec time) {}; //needed for HDEEM
	static void fillTime(timespec &time) { clock_gettime(CLOCK_REALTIME, &time); };
	static void deleteSamples() {};
	static std::string getSourceLabel() {	return "Jetson";};

private:
	static std::string filename;
	static std::vector<jetsonSample> _samples;
};


}



#endif /* SRC_WRAPPER_JETSONWRAPPER_H_ */
