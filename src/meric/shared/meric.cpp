
#include "meric.h"

using namespace meric;

SharedMeric::SharedMeric()
{
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
					_storeList.push_back(new TimeStore<HDEEM>(Meric::SAMPLES));
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
	
	if (Meric::MODE == MODE_JETSON)
	{
		_storeList.push_back(new TimeStore<JETSON>(Meric::SAMPLES));
	}
	
	if (Meric::COUNTERS != "none")
	{
		_storeList.push_back(new CountersStore(Meric::COUNTERS, Meric::SAMPLES));
	}
	
	for (int i=0; i<_storeList.size(); i++)
		_storeList[i]->setMPIWrite(true);

	if (Meric::MODE == MODE_JETSON)
		Environment::init(SYS_JETSON, defaultParameter, 0);
	else
		Environment::init(SYS_HASWELL, defaultParameter, 0);
	
	for (int socket=0; socket < NUMOFSOCKETS; socket++)
		currentState.socket[socket] = defaultParameter;



	//wrappers must be initialized after meric instantiation
	//rapl counter expects that environment::init() was already called
	if (Meric::COUNTERS == "perfevent")
		PERFEVENT::init();
	if (Meric::COUNTERS == "papi")
		PAPI::init();
	if (Meric::MODE == MODE_RAPL || Meric::MODE == MODE_BOTH)
	{
		if (Meric::DETAILED)
			RAPL::detailed();
		RAPL::init();
	}
	if (Meric::MODE == MODE_JETSON)
		JETSON::init();
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



