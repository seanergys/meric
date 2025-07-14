#pragma once

#ifndef __CPU_AFFINITY_MAP_H__
#define __CPU_AFFINITY_MAP_H__

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

    virtual void CollectMachineInfo() = 0;
    virtual void ComputeAffinity() = 0;
    virtual void BindThreads() = 0;

    virtual const CoreToThreadMap_t &GetCoreToThreadMap() const = 0;

protected:
};

}
}


#endif // __CPU_AFFINITY_MAP_H__
