#include "reserved_memory_manager.h"
#include "meric_ext.h"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include "../basis/utils.h"

// Static member variable definitions
std::unordered_map<int, std::unordered_map<int, ReservedMemoryManager::DomainMemoryMap>> ReservedMemoryManager::domain_data; // domain_id -> page -> memory_map
std::unordered_map<int, std::vector<ExtlibEnergyTimeStamp>> ReservedMemoryManager::metadata;                                 // page -> timestamp metadata
std::unordered_map<int, int> ReservedMemoryManager::entry_length;                                                            // page -> num of entries
size_t ReservedMemoryManager::reserved_memory_size = 0;
bool ReservedMemoryManager::is_enabled = false;

int ReservedMemoryManager::count_total_empty_slots_before(int page)
{
    int empty_slots = 0;
    for (auto &&i : entry_length)
    {
        if (i.first < page)
            empty_slots += (reserved_memory_size - i.second);
    }
    std::cout << "total empty slots :" << empty_slots << " before page :" << page << "\n";
    return empty_slots;
}

void ReservedMemoryManager::initialize(size_t memory_size, const ExtlibEnergyTimeStamp *template_timestamp)
{
    if (is_enabled)
        return;

    reserved_memory_size = memory_size;
    is_enabled = true;

    int page = 0;
    metadata[page].reserve(reserved_memory_size);
    entry_length[page] = 0;

    printf("[INFO]: Using reserved memory for %zu elements. [OK]\n", metadata[page].capacity());

    // Initialize domain data based on template
    for (int domain_index = 0; domain_index < EXTLIB_NUM_DOMAINS; ++domain_index)
    {
        if (template_timestamp->domain_data[domain_index].arr_size == 0)
            continue;

        DomainMemoryMap map;
        map.arr_size = template_timestamp->domain_data[domain_index].arr_size;
        size_t total_elements = map.arr_size * reserved_memory_size;

        // Allocate flat arrays for all timestamps
        map.counter_value = (uint64_t *)calloc(total_elements, sizeof(uint64_t));
        map.energy_per_counter = (double *)calloc(total_elements, sizeof(double));
        map.counter_name = (char **)calloc(map.arr_size, sizeof(char *));

        if (!map.counter_value || !map.energy_per_counter || !map.counter_name)
        {
            perror("[ERROR]: Failed to allocate reserved memory for domain arrays");
            exit(EXIT_FAILURE);
        }

        // Copy counter names
        for (size_t j = 0; j < map.arr_size; j++)
        {
            map.counter_name[j] = (char *)calloc(MAX_DOMAIN_NAME_LENGTH, sizeof(char));
            if (!map.counter_name[j])
            {
                perror("[ERROR]: Failed to allocate memory for counter_name string");
                exit(EXIT_FAILURE);
            }

            strncpy(map.counter_name[j], template_timestamp->domain_data[domain_index].counter_name[j], MAX_DOMAIN_NAME_LENGTH - 1);
            map.counter_name[j][MAX_DOMAIN_NAME_LENGTH - 1] = '\0';
        }

        domain_data[domain_index][page] = std::move(map);
    }
}

ExtlibEnergyTimeStamp *ReservedMemoryManager::allocate_timestamp()
{
    if (!is_enabled)
        return nullptr;

    // Look for cleared slots first
    for (auto &entry : metadata)
    {
        int page_id = entry.first;
        auto &vec = entry.second;

        for (size_t i = 0; i < vec.size(); ++i)
        {
            if (vec[i].id == 0)
            {
                std::cout << "Reusing slot: page=" << page_id << ", index=" << i << "\n";
                entry_length[page_id] += 1;
                return initialize_timestamp(&vec[i], page_id, i);
            }
        }
    }

    // Find first non-full page
    for (auto &entry : metadata)
    {
        int page_id = entry.first;
        auto &vec = entry.second;

        if (vec.size() < reserved_memory_size)
        {
            std::cout << "Appending to page: " << page_id << "\n";
            vec.emplace_back();
            entry_length[page_id] += 1;
            return initialize_timestamp(&vec.back(), page_id, vec.size() - 1);
        }
    }

    // Create new page
    return create_new_page();
}

void ReservedMemoryManager::deallocate_timestamp(ExtlibEnergyTimeStamp *timestamp)
{
    if (!is_enabled || !timestamp)
        return;

    int page = timestamp->reserved_memory_page;

    // Clear domain data
    for (int domain_id = 0; domain_id < EXTLIB_NUM_DOMAINS; domain_id++)
    {
        size_t arr_size = timestamp->domain_data[domain_id].arr_size;
        if (arr_size == 0)
            continue;

        if (timestamp->domain_data[domain_id].counter_value)
        {
            memset(timestamp->domain_data[domain_id].counter_value, 0, arr_size * sizeof(uint64_t));
        }
        if (timestamp->domain_data[domain_id].energy_per_counter)
        {
            memset(timestamp->domain_data[domain_id].energy_per_counter, 0, arr_size * sizeof(double));
        }
    }

    std::cout << "\t before removal: page: " << page << " length: " << entry_length[page] << "\n";

    // Check if we should remove the page
    if (should_remove_page(page))
    {
        remove_page(page);
    }
    else
    {
        entry_length[page] -= 1;
        clear_timestamp(timestamp);
    }
}

ReservedMemoryManager::DomainMemoryMap *ReservedMemoryManager::get_domain_map(int domain_id, int page)
{
    if (domain_data.count(domain_id) == 0 || domain_data[domain_id].count(page) == 0)
    {
        return nullptr;
    }
    return &domain_data[domain_id][page];
}

void ReservedMemoryManager::get_domain_arrays(int domain_id, int page, size_t offset,
                                              uint64_t **counter_values, char ***counter_names,
                                              double **energy_per_counter, size_t *arr_size)
{
    auto *domain_map = get_domain_map(domain_id, page);
    if (domain_map)
    {
        *counter_values = &domain_map->counter_value[offset];
        *counter_names = domain_map->counter_name;
        *energy_per_counter = &domain_map->energy_per_counter[offset];
        *arr_size = domain_map->arr_size;
    }
    else
    {
        *counter_values = nullptr;
        *counter_names = nullptr;
        *energy_per_counter = nullptr;
        *arr_size = 0;
    }
}

void ReservedMemoryManager::clear()
{
    // Free all allocated memory
    for (auto &domain_entry : domain_data)
    {
        for (auto &page_entry : domain_entry.second)
        {
            DomainMemoryMap &map = page_entry.second;

            // Free counter names (only for the first page since they're shared)
            if (page_entry.first == 0 && map.counter_name)
            {
                for (size_t i = 0; i < map.arr_size; ++i)
                {
                    if (map.counter_name[i])
                    {
                        free(map.counter_name[i]);
                    }
                }
                free(map.counter_name);
            }

            // Free counter values and energy arrays
            if (map.counter_value)
            {
                free(map.counter_value);
            }
            if (map.energy_per_counter)
            {
                free(map.energy_per_counter);
            }
        }
    }

    domain_data.clear();
    metadata.clear();
    entry_length.clear();
    is_enabled = false;
}

ExtlibEnergyTimeStamp *ReservedMemoryManager::initialize_timestamp(ExtlibEnergyTimeStamp *ptr, size_t page_id, size_t index)
{
    std::memset(ptr, 0, sizeof(ExtlibEnergyTimeStamp));

    ptr->reserved_memory_page = page_id;
    ptr->reserved_memory_index = index;

    // Initialize domain IDs
    for (size_t d = 0; d < EXTLIB_NUM_DOMAINS; ++d)
    {
        ptr->domain_data[d].domain_id = (1 << d);
    }

    struct timespec tt;
    clock_gettime(CLOCK_REALTIME, &tt);
    ptr->id = meric::mutils::timeToDouble(tt);

    return ptr;
}

ExtlibEnergyTimeStamp *ReservedMemoryManager::create_new_page()
{
    size_t new_page_id = metadata.size();
    std::cout << "Creating new page: " << new_page_id << "\n";

    auto &new_vec = metadata[new_page_id];
    new_vec.reserve(reserved_memory_size);
    new_vec.emplace_back();
    entry_length[new_page_id] = 1;

    // Create domain maps for new page
    for (auto &&i : domain_data)
    {
        int domain_id = i.first;
        auto &domain_map = i.second;

        DomainMemoryMap map;
        map.arr_size = domain_map[0].arr_size;
        size_t total_elements = map.arr_size * reserved_memory_size;

        map.counter_value = (uint64_t *)calloc(total_elements, sizeof(uint64_t));
        map.energy_per_counter = (double *)calloc(total_elements, sizeof(double));
        map.counter_name = domain_map[0].counter_name; // Share counter names

        if (!map.counter_value || !map.energy_per_counter || !map.counter_name)
        {
            perror("[ERROR]: Failed to allocate reserved memory for domain arrays");
            exit(EXIT_FAILURE);
        }

        domain_data[domain_id][new_page_id] = std::move(map);
    }

    return initialize_timestamp(&new_vec.back(), new_page_id, 0);
}

bool ReservedMemoryManager::should_remove_page(int page)
{
    return page != 0 &&
           entry_length.at(page) == 1 &&
           page == (int)(entry_length.size() - 1) &&
           count_total_empty_slots_before(page) >= EMPTY_SLOT_THRESHOLD;
}

void ReservedMemoryManager::remove_page(int page)
{
    std::cout << "FREEING PAGE:" << page << "\n";
    entry_length[page] -= 1;

    // Clear timestamp
    auto &vec = metadata[page];
    if (!vec.empty())
    {
        ExtlibEnergyTimeStamp *timestamp = &vec.back();
        clear_timestamp(timestamp);
    }

    metadata.erase(page);

    // Free domain data for this page
    for (int domain_id = 0; domain_id < EXTLIB_NUM_DOMAINS; domain_id++)
    {
        if (domain_data.count(domain_id) == 0)
            continue;

        auto &mmap = domain_data[domain_id][page];
        if (mmap.counter_value)
        {
            free(mmap.counter_value);
            mmap.counter_value = nullptr;
        }
        if (mmap.energy_per_counter)
        {
            free(mmap.energy_per_counter);
            mmap.energy_per_counter = nullptr;
        }

        std::cout << "\t Domain erased from page: " << page << "\n";
        domain_data[domain_id].erase(page);
    }
}

void ReservedMemoryManager::clear_timestamp(ExtlibEnergyTimeStamp *timestamp)
{
    std::cout << "\t\tclear timestamp at index " << timestamp->reserved_memory_index
              << " from page: " << timestamp->reserved_memory_page << "\n";
    memset(timestamp, 0, sizeof(ExtlibEnergyTimeStamp));
    timestamp->id = 0;
    timestamp->reserved_memory_index = -1;
    timestamp->reserved_memory_page = -1;
}
