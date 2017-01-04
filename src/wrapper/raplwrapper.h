
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

private:
		//list of counters
	static std::map<std::string, long> counters;
	static double energyUnit;
	static std::string suffix;
	static int numOfDevices;
	static bool detailedMode;

#else
public:
	static void init() {};
	static void close() {};
	static void getValues(std::map<std::string, long> & record) {std::cerr << "RAPL::getValues() TODO\n"; };
	static void initMap(std::map<std::string, long> & dict);
	static void detailed() {detailedMode = true;};
private:
	static std::string suffix;
	static int numOfDevices;
	static bool detailedMode;
#endif
};

}



#endif /* SRC_WRAPPER_RAPLWRAPPER_H_ */
