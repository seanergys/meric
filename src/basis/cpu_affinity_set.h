#pragma once

#ifndef __CPU_AFFINITY_SET_H__
#define __CPU_AFFINITY_SET_H__

#include <cstdint>
#include <algorithm>
#include <ostream>
#include <array>
#include <vector>

namespace meric {
namespace cpu_affinity_set {

template<typename T, typename E>
class CPUSetExpr_t {
public:
    static constexpr bool is_leaf = false;
    
    T GetElem(size_t i) const { return static_cast<const E &>(*this).GetElem(i); }
    size_t GetSize() const { return static_cast<const E &>(*this).GetSize(); }
    
    size_t Count() const {
        size_t count = 0;
        for(size_t i = 0; i < GetSize(); ++i)
            count += __builtin_popcountg(GetElem(i));
        return count;
    }
    
    bool operator[](size_t i) const {
        if(i >= GetSize() * sizeof(T) * 8)
            return false;
        
        return ((GetElem(i / (sizeof(T) * 8)) >> (i % (sizeof(T) * 8))) & T(1)) != T(0);
    }
    
    template<typename F>
    void IterateBits(F &&f, bool value = true) const {
        for(size_t i = 0; i < GetSize(); ++i)
        {
            T elem = value ? GetElem(i) : (~GetElem(i));
            while(elem != 0)
            {
                size_t offset = __builtin_ctzg(elem);
                elem = elem >> (offset + 1);
                
                f(i*sizeof(T) + offset);
            }
        }
        
    }
    
    friend std::ostream &operator<<(std::ostream &os, const CPUSetExpr_t &obj) {
        for(size_t i = 0; i < obj.GetSize() * sizeof(T) * 8; ++i)
        {
            os << ((obj.GetElem(i / (sizeof(T) * 8)) >> (i % (sizeof(T) * 8))) & 1);
        }
        
        return os;
    }
};

template<typename T, typename D>
class CPUSet_t : public CPUSetExpr_t<T, D> {
public:
    typedef T ElemType_t;
    static constexpr bool is_leaf = true;
    
    void Clear() {
        std::fill(&GetElem(0), &GetElem(0) + GetSize(), T(0));
    }
    
    void Set() {
        std::fill(&GetElem(0), &GetElem(0) + GetSize(), ~T(0));
    }
    
    void Set(size_t i, bool val = true) {
        if(i >= GetSize() * sizeof(T) * 8)
            return;
        
        const T mask = T(1) << (i % (sizeof(T) * 8));
        T &elem = GetElem(i / (sizeof(T) * 8));
        
        elem = val ? (elem | mask) : (elem & ~mask);
    }
    
    operator cpu_set_t*() { return reinterpret_cast<cpu_set_t *>(&GetElem(0)); }
    operator const cpu_set_t*() const { return reinterpret_cast<cpu_set_t *>(&GetElem(0)); }
    
    ElemType_t GetElem(size_t i) const { return static_cast<const D &>(*this).GetElem(i); }
    ElemType_t &GetElem(size_t i) { return static_cast<D &>(*this).GetElem(i); }
    size_t GetSize() const { return static_cast<const D &>(*this).GetSize(); }
    size_t GetByteSize() const { return GetSize()*sizeof(T); }
};

template<typename T, typename E, typename F>
class CPUSetExprUn_t : public CPUSetExpr_t<T, CPUSetExprUn_t<T, E, F> > {
public:
    static constexpr bool is_leaf = false;
    
    CPUSetExprUn_t(const E &u, const F &f)
        : m_u(u), m_f(f)
    {}
    
    T GetElem(size_t i) const { return m_f(m_u.GetElem(i)); }
    size_t GetSize() const { return m_u.GetSize(); }

protected:
    typename std::conditional<E::is_leaf, const E &, const E>::type m_u;
    const F m_f;
};

template<typename T, typename E1, typename E2, typename F>
class CPUSetExprBin_t : public CPUSetExpr_t<T, CPUSetExprBin_t<T, E1, E2, F> > {
public:
    static constexpr bool is_leaf = false;
    
    CPUSetExprBin_t(const E1 &u, const E2 &v, const F &f)
        : m_u(u), m_v(v), m_f(f)
    {}
    
    T GetElem(size_t i) const { return m_f(m_u.GetElem(i), m_v.GetElem(i)); }
    size_t GetSize() const { return m_v.GetSize(); }

protected:
    typename std::conditional<E1::is_leaf, const E1 &, const E1>::type m_u;
    typename std::conditional<E2::is_leaf, const E2 &, const E2>::type m_v;
    const F m_f;
};

template<typename T, typename E>
CPUSetExprUn_t<T, E, T (*)(const T &)> operator~(const CPUSetExpr_t<T, E> &u) {
    return CPUSetExprUn_t<T, E, T (*)(const T &)>(static_cast<const E &>(u), +[](const T &a) { return ~a; });
}

template<typename T, typename E1, typename E2>
CPUSetExprBin_t<T, E1, E2, T (*)(const T &, const T &)> operator|(const CPUSetExpr_t<T, E1> &u, const CPUSetExpr_t<T, E2> &v) {
    return CPUSetExprBin_t<T, E1, E2, T (*)(const T &, const T &)>(static_cast<const E1 &>(u), static_cast<const E2 &>(v), +[](const T &a, const T &b) { return a | b; });
}

template<typename T, typename E1, typename E2>
CPUSetExprBin_t<T, E1, E2, T (*)(const T &, const T &)> operator&(const CPUSetExpr_t<T, E1> &u, const CPUSetExpr_t<T, E2> &v) {
    return CPUSetExprBin_t<T, E1, E2, T (*)(const T &, const T &)>(static_cast<const E1 &>(u), static_cast<const E2 &>(v), +[](const T &a, const T &b) { return a & b; });
}

template<typename T, typename E1, typename E2>
CPUSetExprBin_t<T, E1, E2, T (*)(const T &, const T &)> operator^(const CPUSetExpr_t<T, E1> &u, const CPUSetExpr_t<T, E2> &v) {
    return CPUSetExprBin_t<T, E1, E2, T (*)(const T &, const T &)>(static_cast<const E1 &>(u), static_cast<const E2 &>(v), +[](const T &a, const T &b) { return a ^ b; });
}

template<typename T = uint64_t>
class CPUSetDynamic_t : public CPUSet_t<T, CPUSetDynamic_t<T> > {
public:
    typedef T ElemType_t;
    static constexpr bool is_leaf = true;
    
    CPUSetDynamic_t() : CPUSetDynamic_t(nullptr, sizeof(cpu_set_t)) {}
    CPUSetDynamic_t(size_t size) : CPUSetDynamic_t(nullptr, size) {}
    CPUSetDynamic_t(const cpu_set_t &cpuset) : CPUSetDynamic_t(&cpuset, sizeof(cpu_set_t)) {}
    CPUSetDynamic_t(const cpu_set_t *cpuset, size_t size)
        : m_data((size + sizeof(T) - 1) / sizeof(T), 0)
    {
        if(cpuset)
            memcpy(m_data.data(), cpuset, size);
    }
    
    template<typename E>
    CPUSetDynamic_t(const CPUSetExpr_t<T, E> &expr)
        : m_data(expr.GetSize(), 0)
    {
        for(size_t i = 0; i < expr.GetSize(); ++i)
            m_data[i] = expr.GetElem(i);
    }
    
    ElemType_t GetElem(size_t i) const { return m_data[i]; }
    ElemType_t &GetElem(size_t i) { return m_data[i]; }
    size_t GetSize() const { return m_data.size(); }

protected:
    std::vector<T> m_data;
};

template<typename T = uint64_t, size_t S = sizeof(cpu_set_t)>
class CPUSetStatic_t : public CPUSet_t<T, CPUSetStatic_t<T, S> > {
public:
    typedef T ElemType_t;
    static constexpr bool is_leaf = true;
    
    CPUSetStatic_t() : CPUSetStatic_t(nullptr, sizeof(cpu_set_t)) {}
    CPUSetStatic_t(const cpu_set_t &cpuset) : CPUSetStatic_t(&cpuset, sizeof(cpu_set_t)) {}
    CPUSetStatic_t(const cpu_set_t *cpuset, size_t size) {
        std::fill(m_data.begin(), m_data.end(), T(0));
        
        if(cpuset)
            memcpy(m_data.data(), cpuset, std::min(size, m_data.size()*sizeof(T)));
    }
    
    template<typename E>
    CPUSetStatic_t(const CPUSetExpr_t<T, E> &expr)
    {
        for(size_t i = 0; i < expr.GetSize(); ++i)
            m_data[i] = expr.GetElem(i);
    }
    
    ElemType_t GetElem(size_t i) const { return m_data[i]; }
    ElemType_t &GetElem(size_t i) { return m_data[i]; }
    size_t GetSize() const { return m_data.size(); }
    
protected:
    std::array<T, (S + sizeof(T) - 1) / sizeof(T)> m_data;
};

template<typename T = uint64_t>
class CPUSetView_t : public CPUSet_t<T, CPUSetView_t<T> > {
public:
    typedef T ElemType_t;
    static constexpr bool is_leaf = true;
    
    template<typename E>
    CPUSetView_t(E *pData, size_t size)
        : m_pData(reinterpret_cast<T *>(pData)), m_size(size / sizeof(T))
    {}
    
    template<typename E>
    CPUSetView_t operator=(const CPUSetExpr_t<T, E> &expr)
    {
        for(size_t i = 0; i < expr.GetSize(); ++i)
            m_pData[i] = expr.GetElem(i);
        
        return *this;
    }
    
    ElemType_t GetElem(size_t i) const { return m_pData[i]; }
    ElemType_t &GetElem(size_t i) { return m_pData[i]; }
    size_t GetSize() const { return m_size; }
    
protected:
    T *m_pData;
    size_t m_size;
};

template<typename T>
void BitBlockTranspose2D(T *a, size_t stride = 1, bool flipRows = false)
{
    T m, t;
    m = T(~uint64_t(0) >> (64 - 4*sizeof(T)));
    
    for(int j = 4*sizeof(T); j != 0; j >>= 1, m ^= (m << j))
    {
        for(int k = 0; k < 8*sizeof(T); k = (k + j + 1) & ~j)
        {
            if(flipRows)
            {
                t                = (a[(k+j)*stride] ^ (a[k*stride] >> j)) & m;
                a[(k+j)*stride] ^= t;
                a[k*stride]     ^= (t << j);
            }
            else
            {
                t                  = (a[k*stride] ^ (a[(k + j)*stride] >> j)) & m;
                a[k*stride]       ^= t;
                a[(k + j)*stride] ^= (t << j);
            }
        }
    }
}

template<typename T>
void BitTranspose2D(T *out, const T *in, size_t r = 1, size_t c = 1, bool flipRows = false)
{    
    for(size_t i = 0; i < c; ++i)
    {
        for(size_t j = 0; j < r; ++j)
        {
            for(size_t k = 0; k < 8*sizeof(T); ++k)
                out[j*8*sizeof(T)*c + k*c + i] = in[i*8*sizeof(T)*r + k*r + j];
            
            BitBlockTranspose2D(&out[j*8*sizeof(T)*c + i], c, flipRows);
        }
    }
}

template<typename S, typename D>
void ExtractLSBit(const S &src, D &dst)
{
    typedef typename D::ElemType_t Elem_t;
    
    auto GetLSOne = [](Elem_t arg) { return arg == Elem_t(0) ? Elem_t(0) : (Elem_t(1) << (__builtin_ctzg(arg))); };
    
    size_t i;
    for(i = 0; i < src.GetSize(); ++i)
    {
        const Elem_t &val = src.GetElem(i);
        dst.GetElem(i) = GetLSOne(val);
        if(val != 0)
            break;
    }
    
    for(i++; i < dst.GetSize(); ++i)
        dst.GetElem(i) = Elem_t(0);
}

template<typename D, typename M>
void BindFirstFree(D &affinity, M &mask)
{
    if(affinity.Count() == 0)
        return;
    
    auto freeView = ~mask & affinity;
    
    if(freeView.Count() == 0)
        ExtractLSBit(~mask, affinity);
    else
        ExtractLSBit(freeView, affinity);
    
    mask = mask | affinity;
}

}
}

#endif // __CPU_AFFINITY_SET_H__
