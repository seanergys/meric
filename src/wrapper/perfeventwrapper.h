
#ifndef SRC_WRAPPER_PERFEVENTWRAPPER_H_
#define SRC_WRAPPER_PERFEVENTWRAPPER_H_

#ifdef HAVE_LINUX_PERF_EVENT_H
#include <linux/perf_event.h>
#endif
#include <map>
#include <iostream>


namespace meric {

#ifdef HAVE_LINUX_PERF_EVENT_H

class PERFEVENT {

public:

	static void init();
	static void close();
	
	static void getValues(std::map<std::string, long long> & record);
	static void initMap(std::map<std::string, long long> & dict);

private:
	static void start();
	static long openCounter(perf_event_attr attr);
		//list of counters
	static std::map<std::string,long long> counters;

};

#else

class PERFEVENT {

public:
	static void init() {};
	static void close() {};
	static void start() {};
	static void stop() {};
	static void getValues(std::map<std::string, long long> & record) {std::cerr << "PERFEVENT::getValues() TODO\n"; };
	static void initMap(std::map<std::string, long long> & dict);
};


#endif

}



#endif /* SRC_WRAPPER_PERFEVENTWRAPPER_H_ */
