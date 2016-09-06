
#include "store.h"

using namespace meric;

int Store::getRegionID(const std::string &region)
{
	if (_regions.find(region) == _regions.end()) {
		_regions[region] = _regions.size();
	}
	return _regions[region];
}




