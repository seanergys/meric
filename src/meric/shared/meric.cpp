
#include "meric.h"

using namespace meric;



SharedMeric::SharedMeric()
{
	HDEEM::init();
	HDEEM::start();
	Environment::init(defaultParameter, 0);

	if (Meric::DEBUG) {
		_store = new TimeEnergyStore();
		return;
	}
	
	if (Meric::COUNTERS != "none")
	{
		if (Meric::COUNTERS == "papi" || Meric::COUNTERS == "perfevent")
		{
			_store = new CountersStore(Meric::COUNTERS);
			return;
		}
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

SharedMeric::~SharedMeric()
{
	HDEEM::stop();
	HDEEM::close();
	Environment::close();
	delete _store;
}



