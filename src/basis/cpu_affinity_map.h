#pragma once

#ifndef SRC_BASIS_CPU_AFFINITY_MAP_H_
#define SRC_BASIS_CPU_AFFINITY_MAP_H_

#include <cstdint>
#include <map>
#include <vector>
#include <memory>

namespace meric {
namespace cpu_affinity_map {

class CPUAffinityMap_t
{
public:
    typedef std::map<int, std::vector<uint64_t> > CoreToThreadMap_t;

    virtual ~CPUAffinityMap_t() = default;

    virtual void CollectMachineInfo() = 0;
    virtual void ComputeAffinity(const std::string &mode) = 0;

    virtual const CoreToThreadMap_t &GetCoreToThreadMap() const = 0;

protected:
};

}
}


#endif // SRC_BASIS_CPU_AFFINITY_MAP_H_
