
#ifndef SRC_WRAPPER_ENVIRONMENTWRAPPER_H_
#define SRC_WRAPPER_ENVIRONMENTWRAPPER_H_

#ifndef USE_DUMMY

#ifdef HAVE_CPUFREQ_H
//TODO: include correct header
#include "cpufreq.h"
#endif


#endif

namespace meric {

class Environment {

public:
	static void frequency(double value)
	{
#ifdef USE_DUMMY
		std::cout << "set frequency to: " << value << "\n";
#elif HAVE_CPUFREQ_H
		// TODO: call library function to set frequency
#else
		// TODO: empty or error?
#endif
	}
};

}


#endif /* SRC_WRAPPER_ENVIRONMENTWRAPPER_H_ */
