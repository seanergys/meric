
#ifndef SRC_WRAPPER_PERFEVENTWRAPPER_H_
#define SRC_WRAPPER_PERFEVENTWRAPPER_H_

#include <linux/perf_event.h>
#include <map>
#include <iostream>

//counters descriptions	TODO presunout jinam
const std::string COUNTER_AVX = "COUNTER_AVX";
const std::string COUNTER_CAHE_MISSES = "COUNTER_CACHE_MISSES";
const std::string COUNTER_HW_INSTRUCTIONS = "COUNTER_HW_INSTRUCTIONS";


namespace meric {

#ifndef USE_DUMMY

class PERFEVENT {

public:

	static void init();
	static void close();
	static void start();
	static void stop();
	
	static void getValues();


private:
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
	static void getValues() {std::cerr << "PERFEVENT::getValues() TODO\n"; };
};


#endif

}



#endif /* SRC_WRAPPER_PERFEVENTWRAPPER_H_ */
