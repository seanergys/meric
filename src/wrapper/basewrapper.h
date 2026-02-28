#pragma once

#ifndef __SRC_WRAPPER_BASEWRAPPER_H__
#define __SRC_WRAPPER_BASEWRAPPER_H__

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

namespace meric {

class BaseWrapper
{
public:
    struct SourceInfo_t {
        enum Domain_t {
            DOMAIN_NONE = 0,
            DOMAIN_PER_THREAD,
            DOMAIN_PER_CORE,
            DOMAIN_PER_SOCKET,
            DOMAIN_PER_NODE,
            DOMAIN_GLOBAL,
        };

        enum Flags_t {
            FLAG_NONE_BIT          = 0,
            FLAG_ACCUMULATIVE_BIT  = 1 << 0,
            FLAG_DETAILED_ONLY_BIT = 1 << 1,
            FLAG_DERIVED_BIT       = 1 << 2,
        };

        enum Type_t {
            TYPE_NONE = 0,
            TYPE_ENERGY,
            TYPE_POWER,
        };

        SourceInfo_t(Type_t type = TYPE_NONE, Domain_t domain = DOMAIN_NONE, 
            Flags_t flags = FLAG_NONE_BIT)
            : type(type)
            , domain(domain)
            , flags(flags)
        {}

        bool HasFlag(Flags_t flag) const { return (flag & flags) != 0; }

        bool IsAccumulative() const { return HasFlag(FLAG_ACCUMULATIVE_BIT); }
        bool IsDerived() const { return HasFlag(FLAG_DERIVED_BIT); }

        Type_t type;
        Domain_t domain;
        uint32_t flags;
    };

    struct SamplerData_t {
        SamplerData_t(const SourceInfo_t &info = SourceInfo_t())
            : id(0)
            , info(info)
            , first(0), last(0)
            , energy(0.0)
            , min(0.0), max(0.0)
        {}

        size_t id;
        std::string name;
        SourceInfo_t info;

        size_t first, last;
        double energy;
        double min, max;
    };

    typedef std::vector<SamplerData_t> SamplerRecord_t;
};

}

#endif // __SRC_WRAPPER_BASEWRAPPER_H__
