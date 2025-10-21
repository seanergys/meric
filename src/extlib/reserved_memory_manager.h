// ====================================================================
// MERIC ext - Management of Reserved Memory for Energy Measurement
// --------------------------------------------------------------------
// Written to manage reserved memory for energy measurement timestamps in a EXTLIB library.
//
// Author: Osman Yasal <osman.yasal@vsb.cz>
// Version: 1.0.0
// License: BSD-3
// ====================================================================

#ifndef RESERVED_MEMORY_MANAGER_H
#define RESERVED_MEMORY_MANAGER_H

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <ctime>

#include "meric_ext.h"
#include "../basis/cpu_optimisations.h"
#include "../basis/utils.h"

#ifndef EXTLIB_MAX_DOMAIN_NAME_LENGTH
#define EXTLIB_MAX_DOMAIN_NAME_LENGTH 50
#endif

#ifndef EXTLIB_NUM_DOMAINS
#define EXTLIB_NUM_DOMAINS 6
#endif

#ifndef EXTLIB_EMPTY_SLOT_THRESHOLD
#define EXTLIB_EMPTY_SLOT_THRESHOLD 3 // Minimum number of empty slots before considering page removal
#endif

#ifndef EXTLIB_MAX_ALLOWED_EMPTY_PAGES
#define EXTLIB_MAX_ALLOWED_EMPTY_PAGES 1 // only allow 1 fully empty page.
#endif

/*
    --------- ReservedMemoryManager Class ---------

    This class abstracts away the paging system for reserved memory.
    It manages memory pages, energy timestamp allocation, and cleanup.
    All members and methods are static for global access.

    How it works:
        Default page is page 0, never removed unless close sequence is called.
        DomainMemoryPool is a struct that contains pointers for arrays for counter names, values, and energy per energy domain.
        ExtlibEnergyTimeStamp represents a single timestamp with value pointers pointing to memory locations for values.
        When a ExtlibEnergyTimeStamp is allocated with allocate_timestamp(), it creates ExtlibEnergyTimeStamp and its pointers point to the memory locations in DomainMemoryPool and marks the slot as occupied.
        When a ExtlibEnergyTimeStamp is deallocated, it clears (zeros) the values and mark the slot as empty.
        metadata is used for ExtlibEnergyTimeStamp and pointers points to addresses at domain_memory_pool.
        Each energy domains like RAPL,NVML has this setup.
        When a page is full, it creates a new page and allocates memory for the new ExtlibEnergyTimeStamp.
        When a page is empty, it checks if it should be removed based on the ruleset defined in should_remove_page() method.
        Note that allocated page size and indexes are fixed. They do not shrink or extend. they are just used or free.
        allocate_timestamp prioritizes using empty slots over occupying a new slot. if all slots are occupied, it has no chance to create another page with fixed sized reserved memory.
        All pages in the same domain share the domain names array, since it is the same for all of them.
        Below, in DomainMemoryPool, one can see the paging structure.
*/
class ReservedMemoryManager
{
public:
    struct DomainMemoryPool                   //        ---------- PAGE 0 ----------                 ---------- PAGE 1 ----------
    {                                         //   index0  index1   index2   index3   index4  ...    index0  index1   index2   index3   index4 ...
        char **counter_name = nullptr;        // [--------]
        uint64_t *counter_value = nullptr;    // [--------|--------|--------|--------|--------...] [--------|--------|--------|--------|--------...]
        double *energy_per_counter = nullptr; // [--------|--------|--------|--------|--------...] [--------|--------|--------|--------|--------...]
        size_t arr_size = 0;
        size_t sizeof_energy_per_counter = 0; // total size for all timestamps in this page
        size_t sizeof_counter_value = 0;      // total size for all timestamps in this page
        size_t sizeof_counter_name = 0;       // total size for all timestamps in this page
    };
    static bool is_enabled;
    static size_t reserved_memory_size;

#if VERBOSE
public:
#else
private:
#endif
    static std::unordered_map<uint64_t, std::unordered_map<uint64_t, DomainMemoryPool>> domain_memory_pool; // domain_id -> page -> memory_map
    static std::unordered_map<uint64_t, std::vector<ExtlibEnergyTimeStamp>> metadata;                       // page -> timestamp metadata
    static std::unordered_map<uint64_t, uint64_t> entry_length;                                             // page -> num of entries
    static std::unordered_set<uint64_t> empty_page_ids;                                                     // List of empty page IDs for quick access, note that page 0 is NOT added.

public:
    static void initialize(size_t memory_size, const ExtlibEnergyTimeStamp *template_timestamp);
    static ExtlibEnergyTimeStamp *allocate_timestamp(bool is_result_timestamp = false);
    static void deallocate_timestamp(ExtlibEnergyTimeStamp *timestamp);
    static DomainMemoryPool *get_domain_map(uint64_t domain_id, uint64_t page);
    static void shrink_to_fit();
    static void close();

#ifdef VERBOSE
public:
#else
private:
#endif

    ReservedMemoryManager() = delete;
    ReservedMemoryManager(const ReservedMemoryManager &) = delete;
    ReservedMemoryManager &operator=(const ReservedMemoryManager &) = delete;

    static uint64_t count_total_empty_slots_before(uint64_t page);
    static ExtlibEnergyTimeStamp *initialize_timestamp(ExtlibEnergyTimeStamp *ptr, size_t page_id, size_t index);
    static ExtlibEnergyTimeStamp *create_new_page();
    static bool should_remove_page(uint64_t page);
    static void remove_page(uint64_t page);
    static void clear_timestamp(ExtlibEnergyTimeStamp *timestamp);
    static uint64_t getset_new_pageid(uint64_t page = -1); // it will wrap around.
};

#endif // RESERVED_MEMORY_MANAGER_H
