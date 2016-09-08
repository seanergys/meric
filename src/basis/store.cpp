
#include "store.h"

using namespace meric;

int Store::getRegionID(const std::string &region)
{
	if (_regions.find(region) == _regions.end()) {
		size_t size = _regions.size();
		_regions[region] = size;
	}
	return _regions[region];
}

timespec Store::subTimes(const timespec &a, const timespec &b)
{
	timespec diff;
	diff.tv_sec = a.tv_sec - b.tv_sec;
	diff.tv_nsec = a.tv_nsec - b.tv_nsec;
	if (diff.tv_nsec < 0) {
		diff.tv_nsec += 1e9;
		diff.tv_sec--;
	}

	return diff;
}


