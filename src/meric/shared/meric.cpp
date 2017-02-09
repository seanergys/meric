
#include "meric.h"

using namespace meric;



SharedMeric::SharedMeric()
{
	Environment::init(defaultParameter, 0);

	if (Meric::MODE == MODE_HDEEM || Meric::MODE == MODE_BOTH)
	{
		HDEEM::init();
		HDEEM::start(_startTime);


		if (Meric::DEBUG)
		{
			_storeList.push_back(new TimeEnergyStore(Meric::SAMPLES));
		} else {
			if (Meric::CONTINUAL)
			{
				if (Meric::DETAILED) {
					_storeList.push_back(new DetailedTimeStore(Meric::SAMPLES));
				} else {
					_storeList.push_back(new TimeStore(Meric::SAMPLES));
				}
			} else {
				if (Meric::DETAILED)
				{
					_storeList.push_back(new DetailedEnergyStore(Meric::SAMPLES));
				} else {
					_storeList.push_back(new EnergyStore(Meric::SAMPLES));
				}
			}
		}
	}

	if (Meric::MODE == MODE_RAPL || Meric::MODE == MODE_BOTH)
	{
		_storeList.push_back(new CountersStore("rapl", Meric::SAMPLES));
	}
	
	if (Meric::COUNTERS != "none")
	{
		_storeList.push_back(new CountersStore(Meric::COUNTERS, Meric::SAMPLES));
	}
	
	for (int i=0; i<_storeList.size(); i++)
		_storeList[i]->setMPIWrite(true);
}

SharedMeric::~SharedMeric()
{
	Environment::close();
	if (Meric::MODE == MODE_HDEEM || Meric::MODE == MODE_BOTH)
	{
		HDEEM::close();
	}
	
	for(int i=0;i<_storeList.size(); i++)
		delete _storeList[i];
}



