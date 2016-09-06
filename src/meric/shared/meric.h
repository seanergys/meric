
#ifndef SRC_MERIC_SHARED_SHAREDMERIC_H_
#define SRC_MERIC_SHARED_SHAREDMERIC_H_

#include "../meric.h"

namespace meric {

class SharedMeric: public Meric {

public:
	SharedMeric();

	void start(const std::string &region);
	void stop();
};

}

#endif /* SRC_MERIC_SHARED_SHAREDMERIC_H_ */
