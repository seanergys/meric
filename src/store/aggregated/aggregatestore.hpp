
#include "aggregatedstore.h"

using namespace meric;

template <class TStore>
void AggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	RegionStats stats;
	_store.fillRegionStats(stats, iStarts.back(), iEnd);

	ss << "aggregated: " << stats.energy << " (" << stats.duration << "s)\n";
}




