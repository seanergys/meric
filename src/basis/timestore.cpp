
#include "timestore.h"

using namespace meric;

void TimeStore::addStart(const std::string &region)
{
	int regionID = getRegionID(region);

	_records.push_back(TimeRecord(regionID, 0.0));
	_stack.push_back(regionID);
}

void TimeStore::addEnd()
{
	_stack.pop_back();
}

