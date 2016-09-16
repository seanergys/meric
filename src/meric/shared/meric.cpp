
#include "meric.h"

using namespace meric;

SharedMeric::SharedMeric()
{
	HDEEM::init();
	HDEEM::start();

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

SharedMeric::~SharedMeric()
{
	HDEEM::stop();
	HDEEM::close();
	delete _store;
}



