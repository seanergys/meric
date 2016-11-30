
#ifndef SRC_WRAPPER_PAPIWRAPPER_H_
#define SRC_WRAPPER_PAPIWRAPPER_H_
#include "counterwrapper.h"

#include <map>
#include <iostream>


namespace meric {

#if !defined USE_DUMMY && defined HAVE_PAPI_H

class PAPI : public counterwrapper {

public:

	static void init();
	static void close();
	
	static void getValues(std::map<std::string, long> & record);
	static void initMap(std::map<std::string, long> & dict);

private:
		//list of counters
	static std::map<std::string, long> counters;
	static int eventSet;
	static long long * countersValues;

};

#else

class PAPI {

public:
	static void init() {};
	static void close() {};
	static void getValues(std::map<std::string, long> & record) {std::cerr << "PAPI::getValues() TODO\n"; };
	static void initMap(std::map<std::string, long> & dict);
};


#endif

}



#endif /* SRC_WRAPPER_PERFEVENTWRAPPER_H_ */
