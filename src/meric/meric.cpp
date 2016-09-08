
#include "meric.h"

using namespace meric;

bool        Meric::CONTINUAL  = mutils::getenv("MERIC_CONTINUAL", false);
bool        Meric::DETAILED   = mutils::getenv("MERIC_DETAILED", false);
std::string Meric::OUTPUT_DIR = mutils::getenv("MERIC_OUTPU_DIR", "log");


Meric::Meric()
{
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

