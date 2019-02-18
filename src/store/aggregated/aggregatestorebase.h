
#ifndef SRC_STORE_AGGREGATED_AGGREGATESTOREBASE_H_
#define SRC_STORE_AGGREGATED_AGGREGATESTOREBASE_H_

#include "mpi.h"
#include "../store.h"

namespace meric {

template <class TStore>
class AggregateStoreBase: public StoreBase {

public:
/*	AggregateStoreBase(MPI_Comm &comm, bool write): _comm(comm)
	{
		_store = new TStore(write);
		MPI_Comm_rank(_comm, &_rank);
		MPI_Comm_size(_comm, &_size);
	}

	AggregateStoreBase(const std::string counterType, MPI_Comm &comm, bool write): _comm(comm)
	{
		_store = new TStore(counterType, write);
		MPI_Comm_rank(_comm, &_rank);
		MPI_Comm_size(_comm, &_size);
	}
*/

	AggregateStoreBase(MPI_Comm &comm): _comm(comm)
	{
		_store = new TStore(false);
		MPI_Comm_rank(_comm, &_rank);
		MPI_Comm_size(_comm, &_size);
	}

	void addStart(int region, RegionParameters configuration) { _store->addStart(region, configuration); }
	void addEnd(int region) { _store->addEnd(region); };
	double getRegionDuration() { return _store->getRegionDuration(); };
	RegionParameters* getConfiguration(size_t id) { return _store->getConfiguration(id); }

	void save(const std::string outputDir, const std::string outputFilename, std::vector<std::string> regionNames, const std::string prefix = "")
	{
		_store->setRegionsNames(regionNames);
		if (_rank) {
			_store->save([&] (std::vector<size_t> &iStarts, size_t iEnd) {
				saveInterval(iStarts, iEnd);
			});
		} else {
			_store->save(prefix, outputDir, outputFilename, [&] (std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) {
				saveInterval(ss, iStarts, iEnd);
			});
		}
	}
	
	void initStore(	int counter,
					const std::string outputDir,
					const std::string outputFilename,
					const std::string outputFilenameInfo, //node_CF_UnCF_config
					const std::string prefix = "") override 
	{
		this->_store->initStore(counter, outputDir, outputFilename, outputFilenameInfo, prefix);
	}
	
	void loadSchedulerEnvironment()
	{
		if (_rank)
		{
			_jobID = mutils::getenv("SLURM_JOBID", -1);
			if (_jobID == -1)
			{
				_jobID = mutils::getenv("PBS_JOBID", -1);
				if (_jobID != -1)
				{
					_stepID = mutils::getenv("PBS_TASKNUM", -1);
					_nodelist = mutils::getenv("PBS_NODEFILE", std::string(""));
					if (_nodelist != "")
					{
						std::list<std::string> nodes;
						std::ifstream iFile(_nodelist);
						if(iFile.is_open())
						{
							std::string data;
							for(std::string data; std::getline(iFile, data);)
							{
								nodes.push_back(data);
							}
							iFile.close();
						}

						nodes.unique();
						_nodelist = "";
						_nodelist = std::accumulate(std::begin(nodes), std::end(nodes),
							        std::string(),
							        [](std::string &a, std::string &b) {
										return a.empty() ? b : a + ';' + b;
									});

					}
				}
			}
			else
			{
				_stepID = mutils::getenv("SLURM_STEPID", -1);
				_nodelist = mutils::getenv("SLURM_NODELIST", std::string(""));
			}
		}
	}


	~AggregateStoreBase()
	{
		delete _store;
	}

protected:
	virtual void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) =0;
	virtual void saveInterval(std::vector<size_t> &iStarts, size_t iEnd) =0;

	TStore* _store;
	int _rank;
	int _size;
	MPI_Comm _comm;
};

}



#endif /* SRC_STORE_AGGREGATED_AGGREGATESTOREBASE_H_ */
