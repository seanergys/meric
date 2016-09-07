
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




