#include "reserved_memory_manager.h"
#include "meric_ext.h"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include "../basis/utils.h"

// Static member variable definitions
std::unordered_map<int, std::unordered_map<int, ReservedMemoryManager::DomainMemoryPool>> ReservedMemoryManager::domain_memory_pool; // domain_id -> page -> memory_map
std::unordered_map<int, std::vector<ExtlibEnergyTimeStamp>> ReservedMemoryManager::metadata;                                         // page -> timestamp metadata
std::unordered_map<int, int> ReservedMemoryManager::entry_length;                                                                    // page -> num of entries
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
    if (memory_size == 0)
    {
        std::cerr << "[ERROR]: Reserved memory size cannot be zero.\n";
        exit(EXIT_FAILURE);
    }

    ReservedMemoryManager::reserved_memory_size = memory_size;

    int page = 0;
    ReservedMemoryManager::metadata[page].reserve(ReservedMemoryManager::reserved_memory_size);
    ReservedMemoryManager::entry_length[page] = 0;

    printf("[INFO]: Using reserved memory for %zu elements. [OK]\n", ReservedMemoryManager::metadata[page].capacity());

    // Initialize domain data based on template
    for (int domain_index = 0; domain_index < EXTLIB_NUM_DOMAINS; ++domain_index)
    {
        if (template_timestamp->domain_data[domain_index].arr_size == 0)
            continue;

        DomainMemoryPool map;
        map.arr_size = template_timestamp->domain_data[domain_index].arr_size;
        size_t total_elements = map.arr_size * ReservedMemoryManager::reserved_memory_size;

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
            map.counter_name[j] = (char *)calloc(EXTLIB_MAX_DOMAIN_NAME_LENGTH, sizeof(char));
            if (!map.counter_name[j])
            {
                perror("[ERROR]: Failed to allocate memory for counter_name string");
                exit(EXIT_FAILURE);
            }

            strncpy(map.counter_name[j], template_timestamp->domain_data[domain_index].counter_name[j], EXTLIB_MAX_DOMAIN_NAME_LENGTH - 1);
            map.counter_name[j][EXTLIB_MAX_DOMAIN_NAME_LENGTH - 1] = '\0';
        }

        ReservedMemoryManager::domain_memory_pool[domain_index][page] = std::move(map);
    }
}

ExtlibEnergyTimeStamp *ReservedMemoryManager::allocate_timestamp()
{
    int first_non_full_page = -1;

    // Look for cleared slots and track first non-full page
    for (auto &entry : metadata)
    {
        int page_id = entry.first;
        auto &vec = entry.second;

        // Check for cleared slots first (higher priority)
        for (size_t i = 0; i < vec.size(); ++i)
        {
            if (vec[i].id == 0)
            {
                std::cout << "Reusing slot: page=" << page_id << ", index=" << i << "\n";
                entry_length[page_id] += 1;
                std::cout << "page_id:" << page_id << " vec size:" << entry_length[page_id] << "!!\n";
                return initialize_timestamp(&vec[i], page_id, i);
            }
        }

        // Track first non-full page for potential expansion
        if (first_non_full_page == -1 && vec.size() < reserved_memory_size)
        {
            first_non_full_page = page_id;
            // do not break here, I prioritize using empty slots first over expansion.
        }
    }

    // If we found a non-full page, append to it
    if (first_non_full_page != -1)
    {
        std::cout << "Appending to page: " << first_non_full_page << "\n";
        auto &vec = metadata[first_non_full_page];
        vec.emplace_back();
        entry_length[first_non_full_page] += 1;
        std::cout << "page_id:" << first_non_full_page << " vec size:" << entry_length[first_non_full_page] << "!!\n";
        return initialize_timestamp(&vec.back(), first_non_full_page, vec.size() - 1);
    }

    // As last resort, create a new page
    return create_new_page();
}

void ReservedMemoryManager::deallocate_timestamp(ExtlibEnergyTimeStamp *timestamp)
{
    if (!ReservedMemoryManager::is_enabled || !timestamp)
        return;

    // Clear domain data
    for (int domain_id = 0; domain_id < EXTLIB_NUM_DOMAINS; domain_id++)
    {
        size_t arr_size = timestamp->domain_data[domain_id].arr_size;
        if (arr_size == 0)
            continue;

        if (OPT_LIKELY(timestamp->domain_data[domain_id].counter_value))
        {
            memset(timestamp->domain_data[domain_id].counter_value, 0, arr_size * sizeof(uint64_t));
        }
        if (OPT_LIKELY(timestamp->domain_data[domain_id].energy_per_counter))
        {
            memset(timestamp->domain_data[domain_id].energy_per_counter, 0, arr_size * sizeof(double));
        }
    }

    int page = timestamp->reserved_memory_page;

    std::cout << "\t before removal: page: " << page << " length: " << entry_length[page] << "\n";

    // Check if we should remove the page
    entry_length.at(page) -= 1;
    clear_timestamp(timestamp);

    if (should_remove_page(page))
        remove_page(page);
}

ReservedMemoryManager::DomainMemoryPool *ReservedMemoryManager::get_domain_map(int domain_id, int page)
{
    if (domain_memory_pool.count(domain_id) == 0 || domain_memory_pool[domain_id].count(page) == 0)
    {
        return nullptr;
    }
    return &domain_memory_pool[domain_id][page];
}

void ReservedMemoryManager::close()
{
    static bool cntr_names_cleared = false;
    std::cout << "\tclosing sequence\n";

    for (auto &domain_entry : domain_memory_pool)
    {
        for (auto &page_entry : domain_entry.second)
        {
            DomainMemoryPool &map = page_entry.second;

            // Free counter_name strings only once globally
            if (!cntr_names_cleared && map.counter_name)
            {
                for (size_t i = 0; i < map.arr_size; ++i)
                {
                    if (map.counter_name[i])
                    {
                        free(map.counter_name[i]);
                        map.counter_name[i] = nullptr;
                    }
                }
                free(map.counter_name);
                map.counter_name = nullptr;
                cntr_names_cleared = true;
            }

            if (map.counter_value)
            {
                free(map.counter_value);
                map.counter_value = nullptr;
            }

            if (map.energy_per_counter)
            {
                free(map.energy_per_counter);
                map.energy_per_counter = nullptr;
            }
        }
    }

    // Reset global state
    cntr_names_cleared = false;
    domain_memory_pool.clear();
    metadata.clear();
    entry_length.clear();
    ReservedMemoryManager::is_enabled = false;
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
    new_vec.reserve(ReservedMemoryManager::reserved_memory_size);
    new_vec.emplace_back();
    entry_length[new_page_id] = 1;
    std::cout << "page_id:" << new_page_id << " vec size:" << entry_length[new_page_id] << "!!\n";

    // Create domain maps for new page
    for (auto &&i : domain_memory_pool)
    {
        int domain_id = i.first;
        auto &domain_map = i.second;

        DomainMemoryPool map;
        map.arr_size = domain_map.at(0).arr_size; // get the first page's arr_size
        size_t total_elements = map.arr_size * ReservedMemoryManager::reserved_memory_size;
        std::cout << "total_elements: " << total_elements << " for domain_id: " << domain_id << "\n";

        map.counter_value = (uint64_t *)calloc(total_elements, sizeof(uint64_t));
        map.energy_per_counter = (double *)calloc(total_elements, sizeof(double));
        map.counter_name = domain_map.at(0).counter_name; // Share counter names with the first page.

        if (!map.counter_value || !map.energy_per_counter || !map.counter_name)
        {
            perror("[ERROR]: Failed to allocate reserved memory for domain arrays");
            exit(EXIT_FAILURE);
        }

        // domain_memory_pool[domain_id][new_page_id] = std::move(map);
        domain_memory_pool.at(domain_id)[new_page_id] = std::move(map);
    }

    return initialize_timestamp(&new_vec.back(), new_page_id, 0);
}

bool ReservedMemoryManager::should_remove_page(int page)
{
    // if it is not the default page (page 0)
    // if there is no remaining entry in the page
    // if you are the last page.
    // if previous pages has empty entry slots (for optimization) > than the threshold.
    // remove the page.
    return page != 0 &&
           entry_length.at(page) == 0 &&
           //    page == (int)(entry_length.size() - 1) &&
           count_total_empty_slots_before(page) >= EMPTY_SLOT_THRESHOLD;
}

void ReservedMemoryManager::remove_page(int page)
{
    std::cout << "FREEING PAGE:" << page << "\n";

    metadata.erase(page);

    // Free domain memory pool for this page
    for (int domain_id = 0; domain_id < EXTLIB_NUM_DOMAINS; domain_id++)
    {
        if (domain_memory_pool.count(domain_id) == 0)
            continue;

        auto &mmap = domain_memory_pool.at(domain_id).at(page);
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
        // don't free counter_name, since it is shared across all timestamps.
        domain_memory_pool.at(domain_id).erase(page);
        std::cout << "\t Domain: " << domain_id << " erased from page: " << page << "\n";
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
