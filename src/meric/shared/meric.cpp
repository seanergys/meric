
#include "meric.h"
#include "cpu_affinity_map.h"
#include <memory>

using namespace meric;

SharedMeric::SharedMeric()
{
	/*for(const auto &item: Environment::mapCoresToTIDs)
	{
		std::cout << item.first << ": ";
		for(auto t: item.second)
			std::cout << t << " ";
		std::cout << std::endl;
	}*/

	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_HDEEM_SAMPLES) != MONITORS.end())
	{
		HDEEM::init(Meric::DETAILED);
		HDEEM::start(_startTime);

		if (Meric::DEBUG)
		{
			_storeList.push_back(new TimeEnergyStore(Meric::SAMPLES));
		}
		else
		{
			_storeList.push_back(new SamplesStore<HDEEM>(Meric::SAMPLES));
		}
	}
	else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_HDEEM_COUNTERS) != MONITORS.end())
	{
		HDEEM::init(Meric::DETAILED);
		HDEEM::start(_startTime);

		if (Meric::DEBUG)
		{
			_storeList.push_back(new TimeEnergyStore(Meric::SAMPLES));
		}
		else
		{
			_storeList.push_back(new CountersStore<HDEEM, double, double>());
		}
	}

	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_RAPL) != MONITORS.end())
	{
		_storeList.push_back(new CountersStore<RAPL, unsigned long long int, double>());
	}
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_OCC) != MONITORS.end())
	{
		_storeList.push_back(new CountersStore<OCC, uint64_t, uint64_t>());
	}
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_NVML) != MONITORS.end())
	{
		_storeList.push_back(new CountersStore<NVML, unsigned long long, double>());
	}
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_ROCM) != MONITORS.end())
	{
		_storeList.push_back(new CountersStore<ROCM, uint64_t, double>());
	}
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_HWMON_SAMPLES) != MONITORS.end())
	{
		HWMON::start(_startTime);
		_storeList.push_back(new SamplesStore<HWMON>(Meric::SAMPLES));
	}
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_JETSON) != MONITORS.end())
	{
		_storeList.push_back(new SamplesStore<JETSON>(Meric::SAMPLES));
	}
	else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_THUNDER) != MONITORS.end())
	{
		_storeList.push_back(new SamplesStore<THUNDER>(Meric::SAMPLES));
	}
	else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_DAVIDE) != MONITORS.end())
	{
		_storeList.push_back(new SamplesStore<DAVIDE>(Meric::SAMPLES));
	}
	else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_A64FX) != MONITORS.end())
	{
		_storeList.push_back(new CountersStore<A64FX, unsigned long long int, double>());
		//because of using perfevent to read energy counters, the performace counters are disabled
		if (Meric::COUNTERS == "perfevent")
		{
			Meric::COUNTERS = "none";
		}
	}
	
	if (Meric::COUNTERS != "none")
	{
		if (Meric::COUNTERS == "perfevent")
		{
			_storeList.push_back(new CountersStore<PERFEVENT, unsigned long long int, unsigned long long int>());
		}
		else if (Meric::COUNTERS == "papi")
		{
			_storeList.push_back(new CountersStore<PAPI, unsigned long long int, unsigned long long int>());
		}
		else if (Meric::COUNTERS == "msr")
		{
			_storeList.push_back(new CountersStore<MSR, uint64_t, uint64_t>());
		}
	}
	
	if (_storeList.size() > 0 || std::find(MONITORS.begin(), MONITORS.end(), MODE_TIME) != MONITORS.end())
	{
		_storeList.insert(_storeList.begin(), new InfoStore());
	}

	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_JETSON) != MONITORS.end()) //we do not really need to know, that system is Jetson, we may initialize the env as SYS_OTHER
	{
		Environment::init(SYS_JETSON,  defaultParameter, 0);
	}
	else 
	{
		Environment::init(SYS_OTHER, defaultParameter, 0);
	}
	
	for (unsigned int socket=0; socket < Environment::SOCKETS_PER_NODE; socket++)
	{
		currentState.socket[socket] = defaultParameter;
	}

	m_pAffinityMap.reset(new meric::cpu_affinity_map::CPUAffinityMapShared_t<uint64_t>());
	m_pAffinityMap->CollectMachineInfo();
	m_pAffinityMap->ComputeAffinity(THREAD_PIN_MODE);
	Environment::mapCoresToTIDs = m_pAffinityMap->GetCoreToThreadMap();

	//wrappers must be initialized after meric instantiation
	//RAPL + MSR wrappers expects that environment::init() was already called

	for(mode m: MONITORS)
	{
		switch(m)
		{
		case MODE_RAPL:
			RAPL::init(Meric::DETAILED);
			break;
		case MODE_OCC:
			OCC::init(Meric::DETAILED);
			break;
		case MODE_JETSON:
			JETSON::init();
			break;
		case MODE_THUNDER:
			THUNDER::init();
			break;
		case MODE_DAVIDE:
			DAVIDE::init();
			DAVIDE::start(_startTime);
			break;
		case MODE_A64FX:
			A64FX::init();
			break;
		default:
			break;
		}
	}

	if (Meric::COUNTERS == "perfevent")
	{
		PERFEVENT::init();
	}
	else if (Meric::COUNTERS == "papi")
	{
		PAPI::init();
	}
	else if (Meric::COUNTERS == "msr")
	{
		MSR::init();
	}
	
	setMainRegionName();
	start(_mainRegion.c_str());
}

SharedMeric::~SharedMeric()
{
	Environment::close();
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_HDEEM_SAMPLES) != MONITORS.end() || std::find(MONITORS.begin(), MONITORS.end(), MODE_HDEEM_COUNTERS) != MONITORS.end())
	{
		HDEEM::close();
	}
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_OCC) != MONITORS.end())
	{
		OCC::close();
	}
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_DAVIDE) != MONITORS.end())
	{
		DAVIDE::close();
	}
	
	for(unsigned int i=0;i<_storeList.size(); i++)
	{
		delete _storeList[i];
	}
}



