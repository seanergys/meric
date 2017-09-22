
#ifndef SRC_WRAPPER_RAPLWRAPPER_H_
#define SRC_WRAPPER_RAPLWRAPPER_H_

#include <map>
#include <vector>
#include <iostream>


namespace meric {

class RAPL {
#ifdef HAVE_X86_ADAPT_H
public:
	static const int RAPL_BASELINE = 0; // ~70 W should be added in the RADAR config file
	static void init();
	static void close() {};
	
	static void getValues(std::map<std::string, long long> & record);
	static void initMap(std::map<std::string, long long> & dict);
	static void detailed() {detailedMode = true;};
	static unsigned long long getResultValue(long long startValue, long long stopValue, std::string counter);

private:
		//list of counters
	static std::map<std::string, long long> counters;
	static std::map<std::string, double> energyUnit;
	static std::string suffix;
	static int numOfDevices;
	static bool detailedMode;
	static long long COUNTERMAX;

#else
public:
	static const int RAPL_BASELINE = 70; // 70 W
	static void init() {};
	static void close() {};
	static void getValues(std::map<std::string, long long> & record) {std::cerr << "RAPL::getValues() TODO\n"; };
	static void initMap(std::map<std::string, long long> & dict);
	static void detailed() {detailedMode = true;};
	static unsigned long long getResultValue(long long startValue, long long stopValue, std::string counter) {return 0;};
private:
	static std::string suffix;
	static int numOfDevices;
	static bool detailedMode;
	static long long COUNTERMAX;
#endif
};

}



#endif /* SRC_WRAPPER_RAPLWRAPPER_H_ */
