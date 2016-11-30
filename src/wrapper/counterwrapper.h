#ifndef SRC_WRAPPER_COUNTERWRAPPER_H_
#define SRC_WRAPPER_COUNTERWRAPPER_H_

#include <map>

class counterwrapper {

public:

	virtual void init() = 0;
	virtual void close() = 0;
	
	virtual void getValues(std::map<std::string, long> & record) = 0;
	virtual void initMap(std::map<std::string, long> & dict) = 0;
};

#endif /* SRC_WRAPPER_COUNTERWRAPPER_H_ */
