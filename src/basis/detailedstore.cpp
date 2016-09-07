
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
	if (_stack.size()) {
		_records.push_back(DetailedRecord(_stack.back(), 1));
		_stack.pop_back();
	} else {
		std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
	}
}

DetailedStore::~DetailedStore()
{

}




