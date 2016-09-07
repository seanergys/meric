
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
	if (_stack.size()) {
		_records.push_back(TimeRecord(_stack.back(), 1));
		_stack.pop_back();
	} else {
		std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
	}
}

TimeStore::~TimeStore()
{

}
