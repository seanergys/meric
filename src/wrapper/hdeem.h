
#ifndef SRC_WRAPPER_HDEEM_H_
#define SRC_WRAPPER_HDEEM_H_

#include "hdeem.h"

namespace meric {

class HDEEM {

public:
	static void init();
	static void close();
	static void start();
	static void stop();

	static void fillEnergy(double &energy, time_t &time);

private:
	static hdeem_bmc_data_t bmc;
};

}



#endif /* SRC_WRAPPER_HDEEM_H_ */
