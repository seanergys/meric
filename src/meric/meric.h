
#ifndef SRC_MERIC_MERIC_H_
#define SRC_MERIC_MERIC_H_

#include <iostream>
#include "../basis/utils.h"

namespace meric {

class Meric {

public:
	static bool CONTINUAL;
	static std::string OUTPUT_DIR;

	virtual void start(const std::string &region) = 0;
	virtual void stop() = 0;

	Meric() {};
	virtual ~Meric() {};

protected:


};

}



#endif /* SRC_MERIC_MERIC_H_ */
