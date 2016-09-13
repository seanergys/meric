
#include "meric.h"

using namespace meric;

SharedMeric::SharedMeric()
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

SharedMeric::~SharedMeric()
{
	delete _store;
}



