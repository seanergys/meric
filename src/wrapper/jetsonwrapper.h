
#ifndef SRC_WRAPPER_JETSONWRAPPER_H_
#define SRC_WRAPPER_JETSONWRAPPER_H_

#include "../basis/utils.h"
#include "basewrapper.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <ctime>
#include <array>
#include <vector>
#include <cmath>
#include <limits>

#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

namespace meric {

struct jetsonSample {
	double timestamp;
	double DC;
	double GPU;
	double CPU;
};

class JETSON : public BaseWrapper {

public:

	static std::vector<std::string> vrSensorLabel;// = {"GPU", "CPU"}; //"DC",
	static int FREQUENCY;
	static const int PROBE_NUMBER = 3;	//DC + GPU + CPU	//	static const int PROBE_NUMBER = 7; //TODO
	static const std::string SUM_ENERGY_LABEL;

	static void init();
	static int getSamples();
	static void getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> samples);
	static std::string getData(double startTime, double duration, SamplerRecord_t &record);
	static void setTimeAdjustment(timespec time) {}; //needed for HDEEM
	static void fillTime(timespec &time) { clock_gettime(CLOCK_REALTIME, &time); };
	static void deleteSamples() {};
	static std::string getSourceLabel() {	return "JETSON";};
	static size_t getSamplerCount() { return PROBE_NUMBER; }
    static std::string getSamplerLabel(size_t id) { return vrSensorLabel[id]; }

private:
	static std::string filename;
	static std::vector<jetsonSample> _samples;
};


}



#endif /* SRC_WRAPPER_JETSONWRAPPER_H_ */
