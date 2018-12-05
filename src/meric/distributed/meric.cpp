
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

	for (size_t i = 1; i < rCounts.size(); i++)
	{
		rDispl[i] += rDispl[i - 1] + rCounts[i - 1];
	}
	names.resize(rDispl.back() + rCounts.back());

	MPI_Gatherv(name.data(), length * sizeof(char), MPI_BYTE, names.data(), rCounts.data(), rDispl.data(), MPI_BYTE, 0, MPI_COMM_WORLD);

	std::map<std::string, size_t> nodes;
	for (size_t i = 0; i < _MPIglobalSize; i++)
	{
		std::string str(names.begin() + rDispl[i], names.begin() + rDispl[i] + rCounts[i]);
		auto it = nodes.find(str);
		if (it == nodes.end())
		{
			size_t s = nodes.size();
			nodes[str] = s;
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
			HDEEM::init();
			HDEEM::start(_startTime);
		}
	
		if (Meric::DEBUG)
		{
		    /*
		    if(Meric::AGGREGATE) {
		        _store = new AggregateStore<TimeEnergyStore>(_storingProcs);
		    } else {
		        _store = new TimeEnergyStore();
		    }
		    */
		    if(Meric::AGGREGATE)
		    {
//				_storeList.push_back(new DebugAggregateStore<TimeEnergyStore>(_storingProcs, !_MPInodeRank));
				_storeList.push_back(new DebugAggregateStore<TimeEnergyStore>(_storingProcs));
				_storeList.back()->setMPIWrite(!_MPInodeRank);
			} else {
				_storeList.push_back(new TimeEnergyStore(Meric::SAMPLES, !_MPInodeRank));
			}
		} else {
			if (Meric::CONTINUAL)
			{
				if (Meric::DETAILED)
				{
					if (Meric::AGGREGATE)
					{
//						_storeList.push_back(new DetailedAggregateStore<DetailedTimeStore>(_storingProcs, !_MPInodeRank));
						_storeList.push_back(new DetailedAggregateStore<DetailedTimeStore>(_storingProcs));
						_storeList.back()->setMPIWrite(!_MPInodeRank);
					} else {
						_storeList.push_back(new DetailedTimeStore(Meric::SAMPLES, !_MPInodeRank));
					}
				} else {
					if (Meric::AGGREGATE)
					{
//						_storeList.push_back(new AggregateStore<TimeStore>(_storingProcs, !_MPInodeRank));
						_storeList.push_back(new AggregateStore<TimeStore<HDEEM>>(_storingProcs));
						_storeList.back()->setMPIWrite(!_MPInodeRank);
					} else {
						_storeList.push_back(new TimeStore<HDEEM>(Meric::SAMPLES, !_MPInodeRank));
					}
				}
			} else {
				if (Meric::DETAILED)
				{
					if (Meric::AGGREGATE)
					{
//						_storeList.push_back(new DetailedAggregateStore<DetailedEnergyStore>(_storingProcs, !_MPInodeRank));
						_storeList.push_back(new DetailedAggregateStore<DetailedEnergyStore>(_storingProcs));
						_storeList.back()->setMPIWrite(!_MPInodeRank);
					} else {
						_storeList.push_back(new DetailedEnergyStore(Meric::SAMPLES, !_MPInodeRank));
					}
				} else {
					if (Meric::AGGREGATE)
					{
						//_storeList.push_back(new AggregateStore<EnergyStore>(_storingProcs));
						_storeList.push_back(new CountersAggregateStore<CountersStore<HDEEM, double>>(_storingProcs));
						_storeList.back()->setMPIWrite(!_MPInodeRank);
					} else {
						//_storeList.push_back(new EnergyStore(Meric::SAMPLES, !_MPInodeRank));
						_storeList.push_back(new CountersStore<HDEEM, double>(Meric::SAMPLES, !_MPInodeRank));
					}
				}
			}
		}
	}
	
	if (Meric::MODE == MODE_RAPL || Meric::MODE == MODE_BOTH)
	{
		if (Meric::AGGREGATE)
		{
			_storeList.push_back(new CountersAggregateStore<CountersStore<RAPL, unsigned long long int>>(_storingProcs));
			_storeList.back()->setMPIWrite(!_MPInodeRank);
		}
		else
			_storeList.push_back(new CountersStore<RAPL, unsigned long long int>(Meric::SAMPLES, !_MPInodeRank));
	}
	
	if (Meric::MODE == MODE_JETSON && Meric::CONTINUAL)
	{
		if (Meric::AGGREGATE)
		{
			_storeList.push_back(new AggregateStore<TimeStore<JETSON>>(_storingProcs));
			_storeList.back()->setMPIWrite(!_MPInodeRank);
		}
		else
			_storeList.push_back(new TimeStore<JETSON>(Meric::SAMPLES, !_MPInodeRank));
	}
	else if (Meric::MODE == MODE_THUNDER && Meric::CONTINUAL)
	{
		MPI_Comm_split(MPI_COMM_WORLD, _MPIglobalRank, 0, &_supportComm);	//_supportComm currently used by THUNDER only, so the split is here to make it obvious
		_storeList.push_back(new AggregateStore<TimeStore<THUNDER>>(_supportComm));
		_storeList.back()->setMPIWrite(!_MPIglobalRank);
	}
	else if (Meric::MODE == MODE_DAVIDE && Meric::CONTINUAL)
	{
		if (Meric::AGGREGATE)
		{
			_storeList.push_back(new AggregateStore<TimeStore<DAVIDE>>(_storingProcs));
			_storeList.back()->setMPIWrite(!_MPInodeRank);
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
				_storeList.push_back(new CountersAggregateStore<CountersStore<PERFEVENT, unsigned long long int>>(_worldCopy));
			}
			else
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<PERFEVENT, unsigned long long int>>(_currentNode));
			}
			_storeList.back()->setMPIWrite(true);
		}
		else if (Meric::COUNTERS == "papi")
		{
			if (Meric::AGGREGATE)
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<PAPI, unsigned long long int>>(_worldCopy));
			}
			else
			{
				_storeList.push_back(new CountersAggregateStore<CountersStore<PAPI, unsigned long long int>>(_currentNode));
			}
			_storeList.back()->setMPIWrite(true);
		}
	}
	
	if (_storeList.size() > 0 || Meric::MODE == MODE_TIME)
	{
		if(Meric::AGGREGATE)
		{
			_storeList.insert(_storeList.begin(), new InfoAggregateStore<InfoStore>(_storingProcs));
			_storeList.front()->setMPIWrite(!_MPInodeRank);
		}
		else
			_storeList.insert(_storeList.begin(), new InfoStore(!_MPInodeRank));
	}
	
//INIT
	if (Meric::MODE == MODE_JETSON)
		Environment::init(SYS_JETSON,  defaultParameter, _MPInodeRank);
	else if (Meric::MODE == MODE_THUNDER)
		Environment::init(SYS_THUNDER, defaultParameter, _MPInodeRank);
	else if (Meric::MODE == MODE_DAVIDE)
		Environment::init(SYS_DAVIDE,  defaultParameter, _MPInodeRank);
	else
		Environment::init(SYS_HASWELL, defaultParameter, _MPInodeRank);
		//Environment::init(SYS_OTHER, defaultParameter, _MPInodeRank);

	for (int socket=0; socket < Environment::NODE_SIZE_SOCKET; socket++)
		currentState.socket[socket] = defaultParameter;

	//available energy measurement systems on ARM architectures requires short wait to start the measurement - sleep(5)
	if (Meric::MODE == MODE_JETSON && Meric::CONTINUAL)
	{
		if (!_MPInodeRank)
			JETSON::init();
		sleep(5);
	}
	else if (Meric::MODE == MODE_THUNDER && Meric::CONTINUAL)
	{
		if (!_MPIglobalRank)
			THUNDER::init();
		sleep(5);
	}
	else if (Meric::MODE == MODE_DAVIDE && Meric::CONTINUAL)
	{
		if (!_MPIglobalRank)
			DAVIDE::init();
		DAVIDE::start(_startTime);	//TODO << test if it is neccessary to have a start here, also for all procs?
	}
	
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
}

DistributedMeric::~DistributedMeric()
{
	if ((Meric::MODE == MODE_HDEEM || Meric::MODE == MODE_BOTH) && (!_MPInodeRank))
	{
		HDEEM::close();
	}
	else if (Meric::MODE == MODE_DAVIDE && Meric::CONTINUAL && (!_MPInodeRank))
	{
		DAVIDE::close();
	}
	Environment::close();
	MPI_Comm_free(&_currentNode);
	MPI_Comm_free(&_storingProcs);
	MPI_Comm_free(&_worldCopy);

	for(int i=0;i<_storeList.size(); i++)
		delete _storeList[i];
	_storeList.clear();
}




