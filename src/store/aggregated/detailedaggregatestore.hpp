
#include "detailedaggregatestore.h"

using namespace meric;

template <class TStore>
void DetailedAggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	DetailedRegionStats stats;
	_store.fillRegionStats(stats, iStarts.back(), iEnd);

	ss << "detailed aggregated: " << stats.energy[0] << " (" << stats.duration << "s)\n";
}


