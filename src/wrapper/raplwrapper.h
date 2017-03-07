
#ifndef SRC_WRAPPER_RAPLWRAPPER_H_
#define SRC_WRAPPER_RAPLWRAPPER_H_

#include <map>
#include <vector>
#include <iostream>


namespace meric {

class RAPL {
#ifdef HAVE_X86_ADAPT_H
public:

	static void init();
	static void close() {};
	
	static void getValues(std::map<std::string, long> & record);
	static void initMap(std::map<std::string, long> & dict);
	static void detailed() {detailedMode = true;};
	static unsigned long long repairValues(long startValue, long stopValue, std::string counter);

private:
		//list of counters
	static std::map<std::string, long> counters;
	static std::map<std::string, double> energyUnit;
	static std::string suffix;
	static int numOfDevices;
	static bool detailedMode;
	static long long RAPLMAX;

#else
public:
	static void init() {};
	static void close() {};
	static void getValues(std::map<std::string, long> & record) {std::cerr << "RAPL::getValues() TODO\n"; };
	static void initMap(std::map<std::string, long> & dict);
	static void detailed() {detailedMode = true;};
	static unsigned long long repairValues(long startValue, long stopValue, std::string counter) {return 0;};
private:
	static std::string suffix;
	static int numOfDevices;
	static bool detailedMode;
	static long long RAPLMAX;
#endif
};

}



#endif /* SRC_WRAPPER_RAPLWRAPPER_H_ */
