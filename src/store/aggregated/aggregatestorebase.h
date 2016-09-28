
#ifndef SRC_STORE_AGGREGATED_AGGREGATESTOREBASE_H_
#define SRC_STORE_AGGREGATED_AGGREGATESTOREBASE_H_

#include "mpi.h"
#include "../store.h"

namespace meric {

template <class TStore>
class AggregateStoreBase: public StoreBase {

public:
	AggregateStoreBase(MPI_Comm &comm): _comm(comm)
	{
		MPI_Comm_rank(_comm, &_rank);
		MPI_Comm_size(_comm, &_size);
	}

	void addStart(const std::string &region) { _store.addStart(region); }
	void addEnd() { _store.addEnd(); };

	void save(const std::string outputDir, const std::string outputFilename, const std::string prefix = "")
	{
		if (_rank) {
			_store.save([&] (std::vector<size_t> &iStarts, size_t iEnd) {
				saveInterval(iStarts, iEnd);
			});
		} else {
			_store.save(prefix, outputDir, outputFilename, [&] (std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) {
				saveInterval(ss, iStarts, iEnd);
			});
		}
	}

protected:
	virtual void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) =0;
	virtual void saveInterval(std::vector<size_t> &iStarts, size_t iEnd) =0;

	TStore _store;
	int _rank;
	int _size;
	MPI_Comm _comm;
};

}



#endif /* SRC_STORE_AGGREGATED_AGGREGATESTOREBASE_H_ */
