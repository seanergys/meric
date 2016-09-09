
#ifndef SRC_BASIS_DETAILEDTIMESTORE_H_
#define SRC_BASIS_DETAILEDTIMESTORE_H_

#include "timestore.h"

namespace meric {

class DetailedTimeStore: public TimeStore {

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
};

}



#endif /* SRC_BASIS_DETAILEDTIMESTORE_H_ */
