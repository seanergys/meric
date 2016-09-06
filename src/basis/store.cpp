
#include "store.h"

using namespace meric;

void Store::addStart(const std::string &region)
{
	if (_regions.find(region) == _regions.end()) {
		_regions[region] = _regions.size();
	}
	_records.push_back(EnergyRecord(_regions[region], 0.0, 0.0));

	_stack.push_back(_regions[region]);
}

void Store::addEnd()
{
	_stack.pop_back();
}



