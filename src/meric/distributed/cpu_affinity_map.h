#pragma once

#ifndef SRC_MERIC_DISTRIBUTED_CPU_AFFINITY_MAP_H_
#define SRC_MERIC_DISTRIBUTED_CPU_AFFINITY_MAP_H_

#include <mpi.h>

#include "../../basis/cpu_affinity_set.h"
#include "../shared/cpu_affinity_map.h"

namespace meric {
namespace cpu_affinity_map {

template<typename T>
class CPUAffinityMapDist_t : public CPUAffinityMapShared_t<T>
{
public:
    CPUAffinityMapDist_t(int coreCount, int threadCount, MPI_Comm &nodeLocalComm)
        : CPUAffinityMapShared_t<T>(coreCount, threadCount)
    {
        MPI_Comm_dup(nodeLocalComm, &m_nodeLocalComm);
    }
    
    virtual ~CPUAffinityMapDist_t()
    {
        MPI_Op_free(&m_threadIdsMergeOp);
        MPI_Op_free(&m_affinityMapOp);

        MPI_Type_free(&m_threadIdsType);
        MPI_Type_free(&m_affinityMapType);

        MPI_Comm_free(&m_nodeLocalComm);
    }
    
    virtual void CollectMachineInfo()
    {
        typedef typename CPUAffinityMapShared_t<T>::template AffinityMap_t<T, uint8_t> AffinityMap_t;
        
        MPI_Allreduce(MPI_IN_PLACE, &this->m_coreCount, 1, 
                      MPI_INT32_T, MPI_MAX, m_nodeLocalComm);
        MPI_Allreduce(MPI_IN_PLACE, &this->m_threadCount, 1, 
                      MPI_INT32_T, MPI_MAX, m_nodeLocalComm);
        MPI_Comm_size(m_nodeLocalComm, &this->m_rankCount);
        MPI_Comm_rank(m_nodeLocalComm, &this->m_rankIdx);
        
        CPUAffinityMapShared_t<T>::CollectMachineInfo();
        
        MPI_Type_contiguous(this->m_affinityMap.GetByteSize(), MPI_BYTE, &m_affinityMapType);
        MPI_Type_commit(&m_affinityMapType);
        
        MPI_Type_contiguous(this->m_threadIds.size(), MPI_UINT64_T, &m_threadIdsType);
        MPI_Type_commit(&m_threadIdsType);
        
        /*MPI_Type_create_keyval(MPI_TYPE_NULL_COPY_FN, MPI_TYPE_NULL_DELETE_FN, &m_affinityMapKey, nullptr);
        MPI_Type_set_attr(m_affinityMapType, m_affinityMapKey, &this->m_affinityMap);
        
        void *pAffinityMap = nullptr;
        int flag;
        MPI_Type_get_attr(m_affinityMapType, m_affinityMapKey, &pAffinityMap, &flag);*/
        
        MPI_Op_create([](void *pA, void *pB, int *len, MPI_Datatype *pDataType) {
            MPI_Aint lb, ex;
            MPI_Type_get_extent(*pDataType, &lb, &ex);
            
            const size_t size = ex / sizeof(T);
            
            const T *pIn = reinterpret_cast<T *>(pA);
            T *pInOut = reinterpret_cast<T *>(pB);
            
            for(size_t i = 0; i < size; ++i)
                pInOut[i] = pInOut[i] | pIn[i];
            
        }, true, &m_affinityMapOp);
        
        MPI_Op_create([](void *pA, void *pB, int *len, MPI_Datatype *pDataType) {
            MPI_Aint lb, ex;
            MPI_Type_get_extent(*pDataType, &lb, &ex);
            
            const size_t size = ex / sizeof(uint64_t);
            
            const uint64_t *pIn = reinterpret_cast<uint64_t *>(pA);
            uint64_t *pInOut = reinterpret_cast<uint64_t *>(pB);
            
            for(size_t i = 0; i < size; ++i)
                pInOut[i] = pInOut[i] + pIn[i];
        }, true, &m_threadIdsMergeOp);
    }
    
    virtual void ComputeAffinity(const std::string &mode)
    {
        this->CollectAffinity();
        
        MPI_Allreduce(MPI_IN_PLACE, this->m_affinityMap.data.data(), 
                      1, m_affinityMapType, m_affinityMapOp, m_nodeLocalComm);
        MPI_Allreduce(MPI_IN_PLACE, this->m_threadIds.data(), 
                      1, m_threadIdsType, m_threadIdsMergeOp, m_nodeLocalComm);
        
        this->ComputeBinding(mode);
        this->BuildCoreToThreadMap();
    }
    
protected:
    MPI_Comm m_nodeLocalComm;
    MPI_Datatype m_affinityMapType;
    MPI_Datatype m_threadIdsType;
    // int m_affinityMapKey;
    MPI_Op m_affinityMapOp;
    MPI_Op m_threadIdsMergeOp;
};

}
}

#endif // SRC_MERIC_DISTRIBUTED_CPU_AFFINITY_MAP_H_
