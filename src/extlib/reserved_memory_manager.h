#ifndef RESERVED_MEMORY_MANAGER_H
#define RESERVED_MEMORY_MANAGER_H

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <cstddef>
#include "../basis/cpu_optimisations.h"

// Forward declaration
struct ExtlibEnergyTimeStamp;

#ifndef EXTLIB_MAX_DOMAIN_NAME_LENGTH
#define EXTLIB_MAX_DOMAIN_NAME_LENGTH 50
#endif

#ifndef EXTLIB_NUM_DOMAINS
#define EXTLIB_NUM_DOMAINS 6
#endif

#ifndef EMPTY_SLOT_THRESHOLD
#define EMPTY_SLOT_THRESHOLD 3 // Minimum number of empty slots before considering page removal
#endif

/*
    --------- ReservedMemoryManager Class ---------

    This class abstracts away the paging system for reserved memory.
    It manages memory pages, energy timestamp allocation, and cleanup.
    All members and methods are static for global access.

    How it works:
        Default page is page 0, never removed unless close sequence is called.
        DomainMemoryPool is a struct that contains arrays for counter names, values, and energy per energy domain.
        ExtlibEnergyTimeStamp represents a single timestamp with value pointers pointing to memory locations for values.
        When a ExtlibEnergyTimeStamp is allocated with allocate_timestamp(), it creates ExtlibEnergyTimeStamp and its pointers point to the memory locations in DomainMemoryPool and marks the slot as occupied.
        When a ExtlibEnergyTimeStamp is deallocated, it clears (zeros) the values and mark the slot as empty.
        When a page is full, it creates a new page and allocates memory for the new ExtlibEnergyTimeStamp.
        When a page is empty, it checks if it should be removed based on the EMPTY_SLOT_THRESHOLD, if there are enough empty slots before that page, then it removes the page.
        Note that allocated page size and indexes are fixed. They do not shrink or extend. they are just used or free.
        allocate_timestamp priarotizes using empty slots over occupying a new slot. if all slots are occupied, it has no chance to create another page with fixed sized reserved memory.
        All pages in the same domain share the domain names array, since it is the same for all of them.
        Below, in DomainMemoryPool, one can see the paging structure.
*/
class ReservedMemoryManager
{
public:
    struct DomainMemoryPool                   //        ---------- PAGE 0 ----------                 ----------- PAGE 1 ----------       ....
    {                                         //    index1  index2   index3   index4   index5         index1  index2   index3   index4   index5
        char **counter_name = nullptr;        // [--------]
        uint64_t *counter_value = nullptr;    // [--------|--------|--------|--------|--------...] [--------|--------|--------|--------|--------...]
        double *energy_per_counter = nullptr; // [--------|--------|--------|--------|--------...] [--------|--------|--------|--------|--------...]
        size_t arr_size = 0;
    };
    static bool is_enabled;
    static size_t reserved_memory_size;

private:
    static std::unordered_map<int, std::unordered_map<int, DomainMemoryPool>> domain_memory_pool; // domain_id -> page -> memory_map
    static std::unordered_map<int, std::vector<ExtlibEnergyTimeStamp>> metadata;                  // page -> timestamp metadata
    static std::unordered_map<int, int> entry_length;                                             // page -> num of entries

public:
    static void initialize(size_t memory_size, const ExtlibEnergyTimeStamp *template_timestamp);
    static ExtlibEnergyTimeStamp *allocate_timestamp();
    static void deallocate_timestamp(ExtlibEnergyTimeStamp *timestamp);
    static DomainMemoryPool *get_domain_map(int domain_id, int page);
    static void shrink_to_fit();
    static void close();

private:
    ReservedMemoryManager() = delete;
    ReservedMemoryManager(const ReservedMemoryManager &) = delete;
    ReservedMemoryManager &operator=(const ReservedMemoryManager &) = delete;

    static int count_total_empty_slots_before(int page);
    static ExtlibEnergyTimeStamp *initialize_timestamp(ExtlibEnergyTimeStamp *ptr, size_t page_id, size_t index);
    static ExtlibEnergyTimeStamp *create_new_page();
    static bool should_remove_page(int page);
    static void remove_page(int page);
    static void clear_timestamp(ExtlibEnergyTimeStamp *timestamp);
};

#endif // RESERVED_MEMORY_MANAGER_H
