
#include "energystore.h"

using namespace meric;

void EnergyStore::addStart(const std::string &region)
{
	int regionID = getRegionID(region);

	_records.push_back(EnergyRecord(_regions[region], 0.0, 0.0));
	_stack.push_back(_regions[region]);
}

void EnergyStore::addEnd()
{
	_stack.pop_back();
}



