
#ifndef SRC_WRAPPER_NVMLWRAPPER_H_
#define SRC_WRAPPER_NVMLWRAPPER_H_

#include "../basis/utils.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <time.h>
#include <ctime>

#include <vector>
#include <map>
/*
#include <array>


#include <unistd.h>
#include <sys/stat.h>
*/
#include <cmath>


#ifdef HAVE_NVML_H
	#include <nvml.h>
#endif


namespace meric {

/*
struct nvmlSample {
	double timestamp;
	double energy;
//	double DC;
//	double SM; //streaming multiprocesor
//	double memory;
};
*/

class NVML {

public:
	static void init();
	static void close() {};
	
	static void getCounterValues(std::map<std::string, unsigned long long> & record);
	static void initMap(std::map<std::string, unsigned long long> & dict);
	static double getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, std::string counter);
	static std::string getSummaryCounterName();
	static std::string getSourceLabel() {return "NVML";};
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, unsigned long long>& start, std::map<std::string, unsigned long long>& stop, double runtime, std::map <std::string, double> & ratios) {};

	static inline double getUnit(std::string counterName) { return 1000.0; };
private:
		//list of counters
	static std::map<std::string, unsigned long long> counters;
//	static std::string suffix;
//	static bool detailedMode;
	static unsigned long long COUNTERMAX;
//	static std::map<std::string, double> units;
	static std::vector <nvmlDevice_t> GPUhandles;
};


}



#endif /* SRC_WRAPPER_NVMLWRAPPER_H_ */
