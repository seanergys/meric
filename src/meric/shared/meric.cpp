
#include "meric.h"

using namespace meric;



SharedMeric::SharedMeric()
{

	Environment::init(defaultParameter, 0);

	if (Meric::COUNTERS != "none")
	{
		_store = new CountersStore(Meric::COUNTERS);
		return;
	}

	HDEEM::init();
	HDEEM::start();

	if (Meric::DEBUG) {
		_store = new TimeEnergyStore(0);
		return;
	}
	
	if (Meric::CONTINUAL) {
		if (Meric::DETAILED) {
			_store = new DetailedTimeStore();
		} else {
			_store = new TimeStore(0);
		}
	} else {
		if (Meric::DETAILED) {
			_store = new DetailedEnergyStore(0);
		} else {
			_store = new EnergyStore(0);
		}
	}
}

SharedMeric::~SharedMeric()
{
	Environment::close();
	if (Meric::COUNTERS == "none")
	{
		HDEEM::stop();
		HDEEM::close();
	}
	delete _store;
}



