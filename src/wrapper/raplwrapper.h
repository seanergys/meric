
#ifndef SRC_WRAPPER_RAPLWRAPPER_H_
#define SRC_WRAPPER_RAPLWRAPPER_H_

#include <map>
#include <iostream>


namespace meric {

#if !defined USE_DUMMY && defined HAVE_X86_ADAPT_H

class RAPL{

public:

	static void init();
	static void close() {};
	
	static void getValues(std::map<std::string, long> & record);
	static void initMap(std::map<std::string, long> & dict);

private:
		//list of counters
	static std::map<std::string, long> counters;
	static double energyUnit;

};

#else

class PAPI {

public:
	static void init() {};
	static void close() {};
	static void getValues(std::map<std::string, long> & record) {std::cerr << "RAPL::getValues() TODO\n"; };
	static void initMap(std::map<std::string, long> & dict);
};


#endif

}



#endif /* SRC_WRAPPER_RAPLWRAPPER_H_ */
