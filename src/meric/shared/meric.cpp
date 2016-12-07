
#include "meric.h"

using namespace meric;



SharedMeric::SharedMeric()
{

	Environment::init(defaultParameter, 0);

/*----------------------------------------------------------------------------//
//TEST vicero storu
	HDEEM::init();
	HDEEM::start();
	_storeList.push_back(new EnergyStore());
	_storeList.push_back(new CountersStore(Meric::COUNTERS));
	return;
//----------------------------------------------------------------------------*/

	if (Meric::COUNTERS != "none")
	{
//		_store = new CountersStore(Meric::COUNTERS);
		_storeList.push_back(new CountersStore(Meric::COUNTERS));
		return;
	}

	HDEEM::init();
	HDEEM::start();

	if (Meric::DEBUG) {
//		_store = new TimeEnergyStore();
		_storeList.push_back(new TimeEnergyStore());
		return;
	}
	
	if (Meric::CONTINUAL) {
		if (Meric::DETAILED) {
//			_store = new DetailedTimeStore();
			_storeList.push_back(new DetailedTimeStore());
		} else {
//			_store = new TimeStore();
			_storeList.push_back(new TimeStore());
		}
	} else {
		if (Meric::DETAILED) {
//			_store = new DetailedEnergyStore();
			_storeList.push_back(new DetailedEnergyStore());
		} else {
//			_store = new EnergyStore();
			_storeList.push_back(new EnergyStore());
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
	
//	delete _store;
	for(int i=0;i<_storeList.size(); i++)
		delete _storeList[i];
}



