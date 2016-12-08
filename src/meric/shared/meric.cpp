
#include "meric.h"

using namespace meric;



SharedMeric::SharedMeric()
{
	Environment::init(defaultParameter, 0);

	if (Meric::MODE == MODE_HDEEM || Meric::MODE == MODE_BOTH)
	{
		HDEEM::init();
		HDEEM::start();

		if (Meric::DEBUG)
		{
			_storeList.push_back(new TimeEnergyStore());
			return;
		}
	
		if (Meric::CONTINUAL)
		{
			if (Meric::DETAILED) {
				_storeList.push_back(new DetailedTimeStore());
			} else {
				_storeList.push_back(new TimeStore());
			}
		} else {
			if (Meric::DETAILED)
			{
				_storeList.push_back(new DetailedEnergyStore());
			} else {
				_storeList.push_back(new EnergyStore());
			}
		}
	}

	if (Meric::MODE == MODE_RAPL || Meric::MODE == MODE_BOTH)
	{
		_storeList.push_back(new CountersStore("rapl"));
	}
	
	if (Meric::COUNTERS != "none")
	{
		_storeList.push_back(new CountersStore(Meric::COUNTERS));
	}
}

SharedMeric::~SharedMeric()
{
	Environment::close();
	if (Meric::MODE == MODE_HDEEM || Meric::MODE == MODE_BOTH)
	{
		HDEEM::stop();
		HDEEM::close();
	}
	
	for(int i=0;i<_storeList.size(); i++)
		delete _storeList[i];
}



