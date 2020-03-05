
#include "meric.h"

using namespace meric;

SharedMeric::SharedMeric()
{
	if (Meric::MODE == MODE_HDEEM || Meric::MODE == MODE_BOTH)
	{
		HDEEM::init(Meric::DETAILED);
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
				_storeList.push_back(new CountersStore<HDEEM, double>());
			}
		}
	}

	if (Meric::MODE == MODE_RAPL || Meric::MODE == MODE_BOTH)
	{
		_storeList.push_back(new CountersStore<RAPL, unsigned long long int>());
	}
	
	if (Meric::MODE == MODE_JETSON)
	{
		_storeList.push_back(new TimeStore<JETSON>(Meric::SAMPLES));
	}
	else if (Meric::MODE == MODE_THUNDER)
	{
		_storeList.push_back(new TimeStore<THUNDER>(Meric::SAMPLES));
	}
	else if (Meric::MODE == MODE_DAVIDE)
	{
		_storeList.push_back(new TimeStore<DAVIDE>(Meric::SAMPLES));
	}
	
	if (Meric::COUNTERS != "none")
	{
		if (Meric::COUNTERS == "perfevent")
			_storeList.push_back(new CountersStore<PERFEVENT, unsigned long long int>());
		else if (Meric::COUNTERS == "papi")
			_storeList.push_back(new CountersStore<PAPI, unsigned long long int>());
		else if (Meric::COUNTERS == "msr")
			_storeList.push_back(new CountersStore<MSR, uint64_t>());
	}
	
	if (_storeList.size() > 0 || Meric::MODE == MODE_TIME)
		_storeList.insert(_storeList.begin(), new InfoStore());

	if (Meric::MODE == MODE_JETSON) //we do not really need to know, that system is Jetson, we may initialize the env as SYS_OTHER
		Environment::init(SYS_JETSON,  defaultParameter, 0);
	else 
		Environment::init(SYS_OTHER, defaultParameter, 0);
	
	for (unsigned int socket=0; socket < Environment::SOCKETS_PER_NODE; socket++)
		currentState.socket[socket] = defaultParameter;


	//wrappers must be initialized after meric instantiation
	//RAPL + MSR wrappers expects that environment::init() was already called
	if (Meric::COUNTERS == "perfevent")
		PERFEVENT::init();
	if (Meric::COUNTERS == "papi")
		PAPI::init();
	if (Meric::MODE == MODE_RAPL || Meric::MODE == MODE_BOTH)
		RAPL::init(Meric::DETAILED);
	if (Meric::COUNTERS == "msr")
		MSR::init();
	if (Meric::MODE == MODE_JETSON)
		JETSON::init();
	else if (Meric::MODE == MODE_THUNDER)
		THUNDER::init();
	else if (Meric::MODE == MODE_DAVIDE)
	{
		DAVIDE::init();
		DAVIDE::start(_startTime);
	}
	
	std::string applicationMainRegion = std::string(program_invocation_name);
	size_t n = applicationMainRegion.find_last_of("/");
	if (n != std::string::npos)
		applicationMainRegion = applicationMainRegion.substr(n+1);
	n = applicationMainRegion.find_first_of(".");
	if (n != std::string::npos)
		applicationMainRegion = applicationMainRegion.substr(0,n);
	applicationMainRegion +="_static";
	start(applicationMainRegion.c_str());
}

SharedMeric::~SharedMeric()
{
	Environment::close();
	if (Meric::MODE == MODE_HDEEM || Meric::MODE == MODE_BOTH)
	{
		HDEEM::close();
	}
	else if (Meric::MODE == MODE_DAVIDE)
	{
		DAVIDE::close();
	}
	
	for(unsigned int i=0;i<_storeList.size(); i++)
		delete _storeList[i];
}



