
#include "detailedstore.h"

using namespace meric;

void DetailedStore::addStart(const std::string &region)
{
	int regionID = getRegionID(region);

	_records.push_back(DetailedRecord(regionID, 0.0));
	_stack.push_back(regionID);
}

void DetailedStore::addEnd()
{
	_stack.pop_back();
}




