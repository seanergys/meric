
#include "meric.h"
#include <map>

using namespace meric;

DistributedMeric::DistributedMeric()
{
	MPI_Comm_rank(MPI_COMM_WORLD, &_MPIglobalRank);
	MPI_Comm_size(MPI_COMM_WORLD, &_MPIglobalSize);

	int color;
	int length;
	std::vector<char> name(MPI_MAX_PROCESSOR_NAME);
	MPI_Get_processor_name(name.data(), &length);

	std::vector<int> rCounts(_MPIglobalSize);
	std::vector<int> rDispl(_MPIglobalSize);
	std::vector<int> colors(_MPIglobalSize);
	std::vector<char> names;
	MPI_Gather(&length, sizeof(int), MPI_BYTE, rCounts.data(), sizeof(int), MPI_BYTE, 0, MPI_COMM_WORLD);

	for (size_t i = 1; i < rCounts.size(); i++) {
		rDispl[i] += rDispl[i - 1] + rCounts[i - 1];
	}
	names.resize(rDispl.back() + rCounts.back());

	MPI_Gatherv(name.data(), length * sizeof(char), MPI_BYTE, names.data(), rCounts.data(), rDispl.data(), MPI_BYTE, 0, MPI_COMM_WORLD);

	std::map<std::string, size_t> nodes;
	for (size_t i = 0; i < _MPIglobalSize; i++) {
		std::string str(names.begin() + rDispl[i], names.begin() + rDispl[i] + rCounts[i]);
		auto it = nodes.find(str);
		if (it == nodes.end()) {
			size_t s = nodes.size();
			nodes[str] = s;
		}
		colors[i] = nodes[str];
	}

	MPI_Scatter(colors.data(), sizeof(int), MPI_BYTE, &color, sizeof(int), MPI_BYTE, 0, MPI_COMM_WORLD);
	MPI_Comm_split(MPI_COMM_WORLD, color, _MPIglobalRank, &_currentNode);

	MPI_Comm_rank(_currentNode, &_MPInodeRank);
	MPI_Comm_size(_currentNode, &_MPInodeSize);
}




