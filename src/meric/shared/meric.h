
#ifndef SRC_MERIC_SHARED_SHAREDMERIC_H_
#define SRC_MERIC_SHARED_SHAREDMERIC_H_

#include "../meric.h"

namespace meric {

class SharedMeric: public Meric {

public:
	void save()
	{
		HDEEM::stop();
		_store->save();
		HDEEM::start();
	}

	SharedMeric();
	~SharedMeric();
};

}

#endif /* SRC_MERIC_SHARED_SHAREDMERIC_H_ */
