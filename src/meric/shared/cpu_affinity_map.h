#pragma once

#ifndef SRC_MERIC_SHARED_CPU_AFFINITY_MAP_H_
#define SRC_MERIC_SHARED_CPU_AFFINITY_MAP_H_

#include <iostream>
#include <cstdint>
#include <map>
#include <vector>

#include <omp.h>

#include <unistd.h>
#include <sched.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>

#include "../../basis/cpu_affinity_set.h"
#include "../../basis/cpu_affinity_map.h"

namespace meric {
namespace cpu_affinity_map {

using namespace meric::cpu_affinity_set;

template<typename T>
class CPUAffinityMapShared_t : public CPUAffinityMap_t
{
public:
    CPUAffinityMapShared_t(int coreCount, int threadCount)
        : m_coreCount(coreCount/*get_nprocs_conf()*/)
        , m_threadCount(threadCount/*omp_get_max_threads()*/)
        , m_rankCount(1)
        , m_rankIdx(0)
    {
    }
    
    virtual ~CPUAffinityMapShared_t() {}
    
    virtual void CollectMachineInfo()
    {
        m_affinityMap = AffinityMap_t<T, uint8_t>(m_coreCount, m_threadCount, m_rankCount);
        m_threadIds.resize(m_threadCount * m_rankCount);
    }
    
    virtual void ComputeAffinity(const std::string &mode)
    {
        CollectAffinity();
        ComputeBinding(mode);
        BuildCoreToThreadMap();
    }
    
    virtual const CoreToThreadMap_t &GetCoreToThreadMap() const { return m_coreMap; }

protected:
    template<typename C, typename R>
    struct AffinityMap_t {
        static constexpr size_t colsBlockBits = sizeof(C)*8;
        static constexpr size_t rowsBlockBits = sizeof(R)*8;
        
        AffinityMap_t()
        {}
        
        AffinityMap_t(size_t cores, size_t threads, size_t ranks)
            : cols((cores + colsBlockBits - 1) / colsBlockBits)
            , rowsPerRank(threads)
            , rows(((rowsPerRank*ranks + rowsBlockBits - 1) / rowsBlockBits) * rowsBlockBits)
            , ranks(ranks)
            , data(cols*rows, C(0))
        {}
        
        CPUSetView_t<C> GetRowView(size_t row) { return CPUSetView_t<C>(&data[row*cols], cols * sizeof(C)); }
        
        AffinityMap_t<R, C> Transposed() const {
            AffinityMap_t<R, C> out(rows, cols * 8 * sizeof(C), ranks);
            
            BitTranspose2D((R *)out.data.data(), (const R *)data.data(), (sizeof(C) * cols) / sizeof(R), rows / (8*sizeof(R)), true);
            
            return out;
        }
        
        size_t GetByteSize() const { return data.size()*sizeof(C); }
        
        size_t cols;
        size_t rowsPerRank;
        size_t rows;
        size_t ranks;
        
        std::vector<C> data;
    };
    
    void GetThreadsAffinity(AffinityMap_t<T, uint8_t> &affinity, size_t offset) {
        #pragma omp parallel num_threads(m_threadCount)
        {
            const size_t id = omp_get_thread_num();
            CPUSetView_t<T> view = affinity.GetRowView(offset + id);
            sched_getaffinity(0, view.GetByteSize(), view);
        }
    }
    
    void SetThreadsAffinity(AffinityMap_t<T, uint8_t> &affinity, size_t offset) {        
        #pragma omp parallel num_threads(m_threadCount)
        {
            const size_t id = omp_get_thread_num();
            CPUSetView_t<T> view = affinity.GetRowView(offset + id);
            sched_setaffinity(0, view.GetByteSize(), view);
        }
    }
    
    void GetThreadIds(std::vector<uint64_t> &threadIds, size_t offset) {
        #pragma omp parallel num_threads(m_threadCount)
        {
            const size_t id = omp_get_thread_num();
            threadIds[offset + id] = syscall(SYS_gettid);
        }
    }
    
    void BuildCoreToThreadMap() {
        auto coreMap = m_affinityMap.Transposed();

        /*if(m_rankIdx == 0)
        {
            for(const auto &v: m_threadIds)
                std::cout << v << " ";
            std::cout << std::endl;

            for(size_t i = 0; i < coreMap.rows; ++i)
                std::cout << coreMap.GetRowView(i) << std::endl;
        }*/
        
        m_coreMap.clear();
        for(size_t i = 0; i < m_coreCount; ++i)
        {
            auto res = m_coreMap.insert(std::make_pair(int(i), std::vector<uint64_t>()));
            CPUSetView_t<uint8_t> view = coreMap.GetRowView(i);
            view.IterateBits([&](size_t idx) { 
                if(idx >= m_threadIds.size())
                    return;
                
                res.first->second.push_back(m_threadIds[idx]);
            });
        }
    }
    
    void BindFirstFree() {
        CPUSetDynamic_t<T> mask(m_affinityMap.cols * sizeof(T));
        
        for(size_t i = 0; i < m_threadCount*m_rankCount; ++i)
        {
            CPUSetView_t<T> view = m_affinityMap.GetRowView(i);
            meric::cpu_affinity_set::BindFirstFree(view, mask);

            if(mask.Count() >= m_coreCount)
                mask.Clear();
        }
    }

    void BindWithSpread(size_t spread) {
        spread = std::max(spread, size_t(1));

        for(size_t i = 0; i < m_threadCount*m_rankCount; ++i)
        {
            CPUSetView_t<T> view = m_affinityMap.GetRowView(i);
            view.Clear();
            view.Set((i*spread) % m_coreCount, true);
        }
    }
    
    void CollectAffinity() {
        GetThreadsAffinity(m_affinityMap, m_affinityMap.rowsPerRank * m_rankIdx);
        GetThreadIds(m_threadIds, m_affinityMap.rowsPerRank * m_rankIdx);
    }

    void ComputeBinding(const std::string &mode) {
        if(mode == "none")
            return;

        if(mode == "close")
        {
            BindWithSpread(1);
        }
        else if(mode == "spread")
        {
            BindWithSpread(m_coreCount / (m_threadCount*m_rankCount));
        }
        else
        {
            BindFirstFree();
        }

        SetThreadsAffinity(m_affinityMap, m_threadCount * m_rankIdx);
    }
    
    int m_coreCount;
    int m_threadCount;
    int m_rankCount;
    int m_rankIdx;
    std::vector<uint64_t>     m_threadIds;
    AffinityMap_t<T, uint8_t> m_affinityMap;
    CoreToThreadMap_t         m_coreMap;
};

}
}

#endif // SRC_MERIC_SHARED_CPU_AFFINITY_MAP_H_
