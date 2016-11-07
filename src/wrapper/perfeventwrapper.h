
#ifndef SRC_WRAPPER_PERFEVENTWRAPPER_H_
#define SRC_WRAPPER_PERFEVENTWRAPPER_H_

#include <linux/perf_event.h>
#include <map>
#include <iostream>


namespace meric {

#ifndef USE_DUMMY

class PERFEVENT {

public:

	static void init();
	static void close();
	
	static void getValues(std::map<std::string, long> & record);
	static void initMap(std::map<std::string, long> & dict);

private:
	static void start();
	static long openCounter(perf_event_attr attr);
		//list of counters
	static std::map<std::string, long> counters;

};

#else

class PERFEVENT {

public:
	static void init() {};
	static void close() {};
	static void start() {};
	static void stop() {};
	static void getValues(std::map<std::string, long> & record) {std::cerr << "PERFEVENT::getValues() TODO\n"; };
	static void initMap(std::map<std::string, long> & dict);
};


#endif

}



#endif /* SRC_WRAPPER_PERFEVENTWRAPPER_H_ */
