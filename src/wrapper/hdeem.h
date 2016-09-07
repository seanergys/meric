
#ifndef SRC_WRAPPER_HDEEM_H_
#define SRC_WRAPPER_HDEEM_H_

#include <cstdlib>

namespace meric {

class HDEEM {

public:
	static void start();
	static void end();

	static void fillEnergy(size_t &energy, size_t &time);
};

}



#endif /* SRC_WRAPPER_HDEEM_H_ */
