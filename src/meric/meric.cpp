
#include "meric.h"

using namespace meric;

bool        Meric::CONTINUAL  = mutils::getenv("MERIC_CONTINUAL", false);
bool        Meric::DETAILED   = mutils::getenv("MERIC_DETAILED", false);
std::string Meric::OUTPUT_DIR = mutils::getenv("MERIC_OUTPU_DIR", "log");


void Meric::start(const std::string &region)
{
	if (Meric::CONTINUAL) {
		_timeStore.addStart(region);
	} else {
		if (Meric::DETAILED) {
			_detailedStore.addStart(region);
		} else {
			_energyStore.addStart(region);
		}
	}
}

void Meric::stop()
{
	if (Meric::CONTINUAL) {
		_timeStore.addEnd();
	} else {
		if (Meric::DETAILED) {
			_detailedStore.addEnd();
		} else {
			_energyStore.addEnd();
		}
	}
}

