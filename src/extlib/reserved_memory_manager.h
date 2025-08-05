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
    It manages memory pages, timestamp allocation, and cleanup.
    All members and methods are static for global access.
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
    static void get_domain_arrays(int domain_id, int page, size_t offset,
                                  uint64_t **counter_values, char ***counter_names,
                                  double **energy_per_counter, size_t *arr_size);
    static void clear();

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
