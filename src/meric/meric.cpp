
#include "meric.h"

using namespace meric;

bool        Meric::CONTINUAL  = mutils::getenv("MERIC_CONTINUAL" , false);
bool        Meric::DETAILED   = mutils::getenv("MERIC_DETAILED"  , false);
bool        Meric::DEBUG      = mutils::getenv("MERIC_DEBUG"     , false);
std::string Meric::OUTPUT_DIR = mutils::getenv("MERIC_OUTPUT_DIR", "log");


Meric::Meric()
{
	if (Meric::DEBUG) {
		_store = new TimeEnergyStore();
		return;
	}

	if (Meric::CONTINUAL) {
		if (Meric::DETAILED) {
			_store = new DetailedTimeStore();
		} else {
			_store = new TimeStore();
		}
	} else {
		if (Meric::DETAILED) {
			_store = new DetailedEnergyStore();
		} else {
			_store = new EnergyStore();
		}
	}
}

