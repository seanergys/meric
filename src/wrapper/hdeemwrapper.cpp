

#include "hdeemwrapper.h"

using namespace meric;

#ifndef USE_DUMMY

hdeem_bmc_data_t HDEEM::bmc;

#else

size_t HDEEM::energy = 0;

#endif




