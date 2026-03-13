
#include "meric.h"
#include "cpu_affinity_map.h"
#include <map>

using namespace meric;

DistributedMeric::DistributedMeric()
{
	MPI_Comm_rank(MPI_COMM_WORLD, &_MPIglobalRank);
	MPI_Comm_size(MPI_COMM_WORLD, &_MPIglobalSize);
	MPI_Comm_dup(MPI_COMM_WORLD, &_worldCopy);
	int color;
	int length;

//MPI COMM
	std::vector<char> name(MPI_MAX_PROCESSOR_NAME);
	MPI_Get_processor_name(name.data(), &length);
	_nodeName = std::string(name.begin(), name.begin() + length);

	std::vector<int> rCounts(_MPIglobalSize);
	std::vector<int> rDispl(_MPIglobalSize);
	std::vector<int> colors(_MPIglobalSize);
	std::vector<char> names;
	MPI_Gather(&length, sizeof(int), MPI_BYTE, rCounts.data(), sizeof(int), MPI_BYTE, 0, MPI_COMM_WORLD);

	for (unsigned int i = 1; i < rCounts.size(); i++)
	{
		rDispl[i] += rDispl[i - 1] + rCounts[i - 1];
	}
	names.resize(rDispl.back() + rCounts.back());

	MPI_Gatherv(name.data(), length * sizeof(char), MPI_BYTE, names.data(), rCounts.data(), rDispl.data(), MPI_BYTE, 0, MPI_COMM_WORLD);

	std::map<std::string, size_t> nodes;
	for (int i = 0; i < _MPIglobalSize; i++)
	{
		std::string str(names.begin() + rDispl[i], names.begin() + rDispl[i] + rCounts[i]);
		auto it = nodes.find(str);
		if (it == nodes.end())
		{
			nodes[str] = nodes.size();
		}
		colors[i] = nodes[str];
	}

	MPI_Scatter(colors.data(), sizeof(int), MPI_BYTE, &color, sizeof(int), MPI_BYTE, 0, MPI_COMM_WORLD);

	MPI_Comm_split(MPI_COMM_WORLD, color, _MPIglobalRank, &_currentNode);
	MPI_Comm_rank(_currentNode, &_MPInodeRank);
	MPI_Comm_size(_currentNode, &_MPInodeSize);

	MPI_Comm_split(MPI_COMM_WORLD, _MPInodeRank, _MPIglobalRank, &_storingProcs);

	int nodeFirst = (_MPInodeRank) ? 0 : 1;
	MPI_Scan(&nodeFirst, &_nodeId, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	_nodeId--;
	
//STORE
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_HDEEM_SAMPLES) != MONITORS.end())
	{
		if (!_MPInodeRank)
		{
			HDEEM::init(Meric::DETAILED);
			HDEEM::start(_startTime);
		}
	
		if (Meric::DEBUG)
		{
			if(Meric::AGGREGATE)
			{
				_storeList.push_back(new DebugAggregateStore<TimeEnergyStore>(_storingProcs, !_MPInodeRank));
			}
			else
			{
				_storeList.push_back(new TimeEnergyStore(Meric::SAMPLES, !_MPInodeRank));
			}
		}
		else
		{
			if (Meric::AGGREGATE)
			{
				if (Meric::SAMPLES)
					std::cerr << "MERIC WARNING: HDEEM power samples are available in non-aggregate mode only\n";
				_storeList.push_back(new SamplesAggregateStore<SamplesStore<HDEEM>>(_storingProcs, !_MPInodeRank));
			}
			else
			{
				_storeList.push_back(new SamplesStore<HDEEM>(Meric::SAMPLES, !_MPInodeRank));
			}
		}
	}
	else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_HDEEM_COUNTERS) != MONITORS.end())
	{
		if (!_MPInodeRank)
		{
			HDEEM::init(Meric::DETAILED);
			HDEEM::start(_startTime);
		}
	
		if (Meric::DEBUG)
		{
			if(Meric::AGGREGATE)
			{
				_storeList.push_back(new DebugAggregateStore<TimeEnergyStore>(_storingProcs, !_MPInodeRank));
			}
			else
			{
				_storeList.push_back(new TimeEnergyStore(Meric::SAMPLES, !_MPInodeRank));
			}
		}
		else
		{
			if (Meric::AGGREGATE)
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<HDEEM, double, double>>(_storingProcs, !_MPInodeRank));
			}
			else
			{
				_storeList.push_back(new CountersStore<HDEEM, double, double>(!_MPInodeRank));
			}
		}
	}
	
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_RAPL) != MONITORS.end())
	{
		if (Meric::AGGREGATE)
		{
			if(Meric::SAMPLES)
			{
				_storeList.push_back(new SamplesAggregateStore<SamplesStore<RAPL>>(_storingProcs, !_MPInodeRank));
			}
			else
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<RAPL, RAPL::CounterValue_t, double>>(_storingProcs, !_MPInodeRank));
			}
		}
		else
		{
			if(Meric::SAMPLES)
			{
				_storeList.push_back(new SamplesStore<RAPL>(Meric::SAMPLES, !_MPInodeRank));
			}
			else
			{
				_storeList.push_back(new CountersStore<RAPL, RAPL::CounterValue_t, double>(!_MPInodeRank));
			}
		}
	}
	else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_OCC) != MONITORS.end())
	{
		if (Meric::AGGREGATE)
		{
			_storeList.push_back(new CountersAggregateStore<CountersStore<OCC, uint64_t, uint64_t>>(_storingProcs, !_MPInodeRank));
		}
		else
		{
			_storeList.push_back(new CountersStore<OCC, uint64_t, uint64_t>(!_MPInodeRank));
		}
	}

	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_NVML) != MONITORS.end())
	{
		if (Meric::AGGREGATE)
		{
			if(Meric::SAMPLES)
			{
				_storeList.push_back(new SamplesAggregateStore<SamplesStore<NVML>>(_storingProcs, !_MPInodeRank));
			}
			else
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<NVML, unsigned long long, double>>(_storingProcs, !_MPInodeRank));
			}
		}
		else
		{
			if(Meric::SAMPLES)
			{
				_storeList.push_back(new SamplesStore<NVML>(Meric::SAMPLES, !_MPInodeRank));
			}
			else
			{
				_storeList.push_back(new CountersStore<NVML, unsigned long long, double>(!_MPInodeRank));
			}
		}
	}
	
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_ROCM) != MONITORS.end())
	{
		if (Meric::AGGREGATE)
		{
			_storeList.push_back(new CountersAggregateStore<CountersStore<ROCM, uint64_t, double>>(_storingProcs, !_MPInodeRank));
		}
		else
		{
			_storeList.push_back(new CountersStore<ROCM, uint64_t, double>(!_MPInodeRank));
		}
	}
	
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_HWMON) != MONITORS.end())
	{
		if (Meric::AGGREGATE)
		{
			if(Meric::SAMPLES)
			{
				_storeList.push_back(new SamplesAggregateStore<SamplesStore<HWMON>>(_storingProcs, !_MPInodeRank));
			}
			else
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<HWMON, HWMON::CounterValue_t, double>>(_storingProcs, !_MPInodeRank));
			}
		}
		else
		{
			if(Meric::SAMPLES)
			{
				_storeList.push_back(new SamplesStore<HWMON>(Meric::SAMPLES, !_MPInodeRank));
			}
			else
			{
				_storeList.push_back(new CountersStore<HWMON, HWMON::CounterValue_t, double>(!_MPInodeRank));
			}
		}
	}
		
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_JETSON) != MONITORS.end())
	{
		if (Meric::AGGREGATE)
		{
			_storeList.push_back(new SamplesAggregateStore<SamplesStore<JETSON>>(_storingProcs, !_MPInodeRank));
		}
		else
		{
			_storeList.push_back(new SamplesStore<JETSON>(Meric::SAMPLES, !_MPInodeRank));
		}
	}
	else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_THUNDER) != MONITORS.end())
	{
		//_supportComm currently used by THUNDER only, so the split is here to make it obvious
		MPI_Comm_split(MPI_COMM_WORLD, _MPIglobalRank, 0, &_supportComm);
		_storeList.push_back(new SamplesAggregateStore<SamplesStore<THUNDER>>(_supportComm, !_MPIglobalRank));
	}
	else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_DAVIDE) != MONITORS.end())
	{
		if (Meric::AGGREGATE)
		{
			_storeList.push_back(new SamplesAggregateStore<SamplesStore<DAVIDE>>(_storingProcs, !_MPInodeRank));
		}
		else
		{
			_storeList.push_back(new SamplesStore<DAVIDE>(Meric::SAMPLES, !_MPInodeRank));
		}
	}
	else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_A64FX) != MONITORS.end())
	{
		if (Meric::AGGREGATE)
		{
			_storeList.push_back(new CountersAggregateStore<CountersStore<A64FX, unsigned long long int, double>>(_storingProcs, !_MPInodeRank));
		}
		else
		{
			_storeList.push_back(new CountersStore<A64FX, unsigned long long int, double>(!_MPInodeRank));
		}
	}

	if (Meric::COUNTERS != "none")
	{
		if (Meric::COUNTERS == "perfevent")
		{
			if (Meric::AGGREGATE)
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<PERFEVENT, unsigned long long int, unsigned long long int>>(_storingProcs, !_MPInodeRank));
			}
			else
			{
				_storeList.push_back(new CountersStore<PERFEVENT, unsigned long long int, unsigned long long int>(!_MPInodeRank));
			}
		}
		else if (Meric::COUNTERS == "papi")
		{
			if (Meric::AGGREGATE)
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<PAPI, unsigned long long int, unsigned long long int>>(_worldCopy, true));
			}
			else
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<PAPI, unsigned long long int, unsigned long long int>>(_currentNode, true));
			}
		}
		else if (Meric::COUNTERS == "msr")
		{
			if (Meric::AGGREGATE)
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<MSR, uint64_t, uint64_t>>(_storingProcs, !_MPInodeRank));
			}
			else
			{
				_storeList.push_back(new CountersStore<MSR, uint64_t, uint64_t>(!_MPInodeRank));
			}
		}
	}
	
	if (_storeList.size() > 0 || std::find(MONITORS.begin(), MONITORS.end(), MODE_TIME) != MONITORS.end())
	{
		if(Meric::AGGREGATE)
		{
			_storeList.insert(_storeList.begin(), new InfoAggregateStore<InfoStore>(_storingProcs, !_MPInodeRank));
		}
		else
		{
			_storeList.insert(_storeList.begin(), new InfoStore(_storingProcs, !_MPInodeRank));
		}
	}
	
//INIT
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_JETSON) != MONITORS.end()) //we do not really need to know, that system is Jetson, we may initialize the env as SYS_OTHER
	{
		Environment::init(SYS_JETSON,  defaultParameter, _MPInodeRank); 
	}
	else
	{
		Environment::init(SYS_OTHER, defaultParameter, _MPInodeRank);
	}

	for (int socket=0; socket < Environment::SOCKETS_PER_NODE; socket++)
	{
		currentState.socket[socket] = defaultParameter;
	}

	m_pAffinityMap.reset(new meric::cpu_affinity_map::CPUAffinityMapDist_t<uint64_t>(
		Environment::SOCKETS_PER_NODE*Environment::CORES_PER_SOCKET*Environment::THREADS_PER_CORE,
		std::max(defaultParameter.nthreads, 1), _currentNode));
	m_pAffinityMap->CollectMachineInfo();
	m_pAffinityMap->ComputeAffinity(THREAD_PIN_MODE);
	Environment::mapCoresToTIDs = m_pAffinityMap->GetCoreToThreadMap();

/*
	if(nodeFirst)
	{
		for(const auto &item: Environment::mapCoresToTIDs)
		{
			std::cout << item.first << ": ";
			for(auto t: item.second)
				std::cout << t << " ";
			std::cout << std::endl;
		}
	}
*/

	//available energy measurement systems on ARM architectures requires short wait to start the measurement - sleep(5)
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_JETSON) != MONITORS.end())
	{
		if (!_MPInodeRank)
		{
			JETSON::init();
		}
		sleep(5);
	}
	else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_THUNDER) != MONITORS.end())
	{
		if (!_MPIglobalRank)
		{
			THUNDER::init();
		}
		sleep(5);
	}
	else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_DAVIDE) != MONITORS.end())
	{
		if (!_MPInodeRank)
		{
			DAVIDE::init();
		}
		DAVIDE::start(_startTime);
	}
	else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_A64FX) != MONITORS.end() && !_MPInodeRank)
	{
		A64FX::init();
	}
	
	//wrappers must be initialized after meric instantiation
	//RAPL + MSR wrappers expects that environment::init() was already called
	if (Meric::COUNTERS == "perfevent" && !_MPInodeRank)
	{
		PERFEVENT::init(Meric::DETAILED);
	}

	if (Meric::COUNTERS == "papi")
	{
		PAPI::init();
	}

	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_RAPL) != MONITORS.end() && !_MPInodeRank)
	{
		RAPL::init(Meric::DETAILED, Meric::SAMPLING_FREQUENCY);
		RAPL::start(_startTime, Meric::SAMPLES);
	}

	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_HWMON) != MONITORS.end() && !_MPInodeRank)
	{
		HWMON::init(Meric::DETAILED, Meric::SAMPLING_FREQUENCY);
		HWMON::start(_startTime, Meric::SAMPLES);
	}

	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_NVML) != MONITORS.end() && !_MPInodeRank)
	{
		NVML::start(_startTime, Meric::SAMPLES, Meric::SAMPLING_FREQUENCY);
	}

	if (!_MPInodeRank && std::find(MONITORS.begin(), MONITORS.end(), MODE_OCC) != MONITORS.end())
	{
		OCC::init(Meric::DETAILED);
	}

	if (Meric::COUNTERS == "msr")
	{
		MSR::init();
	}
	
	setMainRegionName();
	start(_mainRegion.c_str());
}

DistributedMeric::~DistributedMeric()
{
	if (!_MPInodeRank && (std::find(MONITORS.begin(), MONITORS.end(), MODE_HDEEM_SAMPLES) != MONITORS.end() || std::find(MONITORS.begin(), MONITORS.end(), MODE_HDEEM_COUNTERS) != MONITORS.end()))
	{
		HDEEM::close();
	}
	if (!_MPInodeRank && std::find(MONITORS.begin(), MONITORS.end(), MODE_OCC) != MONITORS.end())
	{
		OCC::close();
	}
	if (!_MPInodeRank && std::find(MONITORS.begin(), MONITORS.end(), MODE_DAVIDE) != MONITORS.end())
	{
		DAVIDE::close();
	}
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_RAPL) != MONITORS.end())
	{
		RAPL::close();
	}
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_NVML) != MONITORS.end())
	{
		NVML::close();
	}
	if (std::find(MONITORS.begin(), MONITORS.end(), MODE_HWMON) != MONITORS.end())
	{
		HWMON::close();
	}

	Environment::close();
	MPI_Comm_free(&_currentNode);
	MPI_Comm_free(&_storingProcs);
	MPI_Comm_free(&_worldCopy);

	for(unsigned int i=0; i<_storeList.size(); i++)
	{
		delete _storeList[i];
	}
	_storeList.clear();
}




