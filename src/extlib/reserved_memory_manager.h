#ifndef RESERVED_MEMORY_MANAGER_H
#define RESERVED_MEMORY_MANAGER_H

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <cstddef>

// Forward declarations
struct ExtlibEnergyTimeStamp;

#ifndef MAX_DOMAIN_NAME_LENGTH
#define MAX_DOMAIN_NAME_LENGTH 50
#endif

#ifndef EXTLIB_NUM_DOMAINS
#define EXTLIB_NUM_DOMAINS 6
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
    struct DomainMemoryMap                    //        ---------- PAGE 0 ----------                 ----------- PAGE 1 ----------       ....
    {                                         //    index1  index2   index3   index4   index5         index1  index2   index3   index4   index5
        char **counter_name = nullptr;        // [--------]
        uint64_t *counter_value = nullptr;    // [--------|--------|--------|--------|--------...] [--------|--------|--------|--------|--------...]
        double *energy_per_counter = nullptr; // [--------|--------|--------|--------|--------...] [--------|--------|--------|--------|--------...]
        size_t arr_size = 0;
    };

private:
    static std::unordered_map<int, std::unordered_map<int, DomainMemoryMap>> domain_data; // domain_id -> page -> memory_map
    static std::unordered_map<int, std::vector<ExtlibEnergyTimeStamp>> metadata;          // page -> timestamp metadata
    static std::unordered_map<int, int> entry_length;                                     // page -> num of entries
    static size_t reserved_memory_size;
    static bool is_enabled;
    static constexpr int EMPTY_SLOT_THRESHOLD = 3;

public:
    // Static methods - no constructor needed
    static void initialize(size_t memory_size, const ExtlibEnergyTimeStamp *template_timestamp);
    static ExtlibEnergyTimeStamp *allocate_timestamp();
    static void deallocate_timestamp(ExtlibEnergyTimeStamp *timestamp);
    static DomainMemoryMap *get_domain_map(int domain_id, int page);
    static void get_domain_arrays(int domain_id, int page, size_t offset,
                           uint64_t **counter_values, char ***counter_names,
                           double **energy_per_counter, size_t *arr_size);
    static void clear();
    static bool enabled() { return is_enabled; }
    static size_t memory_size() { return reserved_memory_size; }

private:
    // Private constructor to prevent instantiation
    ReservedMemoryManager() = delete;
    ReservedMemoryManager(const ReservedMemoryManager&) = delete;
    ReservedMemoryManager& operator=(const ReservedMemoryManager&) = delete;

    static int count_total_empty_slots_before(int page);
    static ExtlibEnergyTimeStamp *initialize_timestamp(ExtlibEnergyTimeStamp *ptr, size_t page_id, size_t index);
    static ExtlibEnergyTimeStamp *create_new_page();
    static bool should_remove_page(int page);
    static void remove_page(int page);
    static void clear_timestamp(ExtlibEnergyTimeStamp *timestamp);
};

#endif // RESERVED_MEMORY_MANAGER_H
