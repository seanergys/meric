
#include "meric.h"
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
	if (Meric::MODE == MODE_HDEEM || Meric::MODE == MODE_BOTH)
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
			} else {
				_storeList.push_back(new TimeEnergyStore(Meric::SAMPLES, !_MPInodeRank));
			}
		} else {
			if (Meric::CONTINUAL)
			{
				if (Meric::AGGREGATE)
				{
					_storeList.push_back(new AggregateStore<TimeStore<HDEEM>>(_storingProcs, !_MPInodeRank));
				} else {
					_storeList.push_back(new TimeStore<HDEEM>(Meric::SAMPLES, !_MPInodeRank));
				}
			} else {
				if (Meric::AGGREGATE)
				{
					_storeList.push_back(new CountersAggregateStore<CountersStore<HDEEM, double>>(_storingProcs, !_MPInodeRank));
				}
				else
				{
					_storeList.push_back(new CountersStore<HDEEM, double>(!_MPInodeRank));
				}
			}
		}
	}
	
	if (Meric::MODE == MODE_RAPL || Meric::MODE == MODE_BOTH)
	{
		if (Meric::AGGREGATE)
		{
			_storeList.push_back(new CountersAggregateStore<CountersStore<RAPL, unsigned long long int>>(_storingProcs, !_MPInodeRank));
		}
		else
			_storeList.push_back(new CountersStore<RAPL, unsigned long long int>(!_MPInodeRank));
	}
	
	if (Meric::MODE == MODE_JETSON)
	{
		if (Meric::AGGREGATE)
		{
			_storeList.push_back(new AggregateStore<TimeStore<JETSON>>(_storingProcs, !_MPInodeRank));
		}
		else
			_storeList.push_back(new TimeStore<JETSON>(Meric::SAMPLES, !_MPInodeRank));
	}
	else if (Meric::MODE == MODE_THUNDER)
	{
		//_supportComm currently used by THUNDER only, so the split is here to make it obvious
		MPI_Comm_split(MPI_COMM_WORLD, _MPIglobalRank, 0, &_supportComm);
		_storeList.push_back(new AggregateStore<TimeStore<THUNDER>>(_supportComm, !_MPIglobalRank));
	}
	else if (Meric::MODE == MODE_DAVIDE)
	{
		if (Meric::AGGREGATE)
		{
			_storeList.push_back(new AggregateStore<TimeStore<DAVIDE>>(_storingProcs, !_MPInodeRank));
		}
		else
			_storeList.push_back(new TimeStore<DAVIDE>(Meric::SAMPLES, !_MPInodeRank));
	}

	if (Meric::COUNTERS != "none")
	{
		if (Meric::COUNTERS == "perfevent")
		{
			if (Meric::AGGREGATE)
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<PERFEVENT, unsigned long long int>>(_worldCopy, true));
			}
			else
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<PERFEVENT, unsigned long long int>>(_currentNode, true));
			}
		}
		else if (Meric::COUNTERS == "papi")
		{
			if (Meric::AGGREGATE)
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<PAPI, unsigned long long int>>(_worldCopy, true));
			}
			else
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<PAPI, unsigned long long int>>(_currentNode, true));
			}
		}
		else if (Meric::COUNTERS == "msr")
		{
			if (Meric::AGGREGATE)
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<MSR, uint64_t>>(_storingProcs, !_MPInodeRank));
			}
			else
				_storeList.push_back(new CountersStore<MSR, uint64_t>(!_MPInodeRank));
		}
	}
	
	if (_storeList.size() > 0 || Meric::MODE == MODE_TIME)
	{
		if(Meric::AGGREGATE)
		{
			_storeList.insert(_storeList.begin(), new InfoAggregateStore<InfoStore>(_storingProcs, !_MPInodeRank));
		}
		else
			_storeList.insert(_storeList.begin(), new InfoStore(!_MPInodeRank));
	}
	
//INIT
	if (Meric::MODE == MODE_JETSON) //we do not really need to know, that system is Jetson, we may initialize the env as SYS_OTHER
		Environment::init(SYS_JETSON,  defaultParameter, _MPInodeRank); 
	else
		Environment::init(SYS_OTHER, defaultParameter, _MPInodeRank);

	for (int socket=0; socket < Environment::SOCKETS_PER_NODE; socket++)
		currentState.socket[socket] = defaultParameter;

	//available energy measurement systems on ARM architectures requires short wait to start the measurement - sleep(5)
	if (Meric::MODE == MODE_JETSON)
	{
		if (!_MPInodeRank)
			JETSON::init();
		sleep(5);
	}
	else if (Meric::MODE == MODE_THUNDER)
	{
		if (!_MPIglobalRank)
			THUNDER::init();
		sleep(5);
	}
	else if (Meric::MODE == MODE_DAVIDE)
	{
		if (!_MPInodeRank)
			DAVIDE::init();
		DAVIDE::start(_startTime);
	}
	
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

DistributedMeric::~DistributedMeric()
{
	if ((Meric::MODE == MODE_HDEEM || Meric::MODE == MODE_BOTH) && (!_MPInodeRank))
	{
		HDEEM::close();
	}
	else if (Meric::MODE == MODE_DAVIDE && (!_MPInodeRank))
	{
		DAVIDE::close();
	}
	Environment::close();
	MPI_Comm_free(&_currentNode);
	MPI_Comm_free(&_storingProcs);
	MPI_Comm_free(&_worldCopy);

	for(unsigned int i=0; i<_storeList.size(); i++)
		delete _storeList[i];
	_storeList.clear();
}




