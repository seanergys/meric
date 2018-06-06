
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
					//_storeList.push_back(new EnergyStore(Meric::SAMPLES));
					_storeList.push_back(new CountersStore<HDEEM, double>(Meric::SAMPLES));
				}
			}
		}
	}

	if (Meric::MODE == MODE_RAPL || Meric::MODE == MODE_BOTH)
	{
		_storeList.push_back(new CountersStore<RAPL, unsigned long long int>(Meric::SAMPLES));
	}
	
	if (Meric::MODE == MODE_JETSON && Meric::CONTINUAL)
	{
		_storeList.push_back(new TimeStore<JETSON>(Meric::SAMPLES));
	}
	else if (Meric::MODE == MODE_THUNDER && Meric::CONTINUAL)
	{
		_storeList.push_back(new TimeStore<THUNDER>(Meric::SAMPLES));
	}
	
	if (Meric::COUNTERS != "none")
	{

		if (Meric::COUNTERS == "perfevent")
			_storeList.push_back(new CountersStore<PERFEVENT, unsigned long long int>(Meric::SAMPLES));
		else if (Meric::COUNTERS == "papi")
			_storeList.push_back(new CountersStore<PAPI, unsigned long long int>(Meric::SAMPLES));
	}
	
	for (int i=0; i<_storeList.size(); i++)
		_storeList[i]->setMPIWrite(true);

	if (Meric::MODE == MODE_JETSON)
		Environment::init(SYS_JETSON,  defaultParameter, 0);
	else if (Meric::MODE == MODE_THUNDER)
		Environment::init(SYS_THUNDER, defaultParameter, 0);
	else
		Environment::init(SYS_OTHER, defaultParameter, 0);
		//Environment::init(SYS_HASWELL, defaultParameter, 0); //this default settings must persist due to many nodes with min uncore frequency limit set to maximum - now it is fixed

	
	for (int socket=0; socket < Environment::NODE_SIZE_SOCKET; socket++)
		currentState.socket[socket] = defaultParameter;


	//wrappers must be initialized after meric instantiation
	//RAPL counter expects that environment::init() was already called
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
	if (Meric::MODE == MODE_JETSON && Meric::CONTINUAL)
		JETSON::init();
	else if (Meric::MODE == MODE_THUNDER && Meric::CONTINUAL)
		THUNDER::init();
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



