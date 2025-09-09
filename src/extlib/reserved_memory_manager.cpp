#include "reserved_memory_manager.h"

// Static member variable definitions
std::unordered_map<uint64_t, std::unordered_map<uint64_t, ReservedMemoryManager::DomainMemoryPool>> ReservedMemoryManager::domain_memory_pool; // domain_id -> page -> memory_map
std::unordered_map<uint64_t, std::vector<ExtlibEnergyTimeStamp>> ReservedMemoryManager::metadata;                                              // page -> timestamp metadata
std::unordered_map<uint64_t, uint64_t> ReservedMemoryManager::entry_length;                                                                    // page -> num of entries
std::unordered_set<uint64_t> ReservedMemoryManager::empty_page_ids;                                                                            // page ids with zero entry

size_t ReservedMemoryManager::reserved_memory_size = 0;
bool ReservedMemoryManager::is_enabled = false;

uint64_t ReservedMemoryManager::count_total_empty_slots_before(uint64_t page)
{
    uint64_t empty_slots = 0;
    for (auto &&i : entry_length)
    {
        if (i.first < page)
            empty_slots += (reserved_memory_size - i.second);
    }
    DEBUG_LOG << "total empty slots :" << empty_slots << " before page-id :" << page << "\n";
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

    uint64_t page = 0;
    ReservedMemoryManager::metadata[page].reserve(ReservedMemoryManager::reserved_memory_size);
    ReservedMemoryManager::entry_length[page] = 0;
    empty_page_ids.insert(page);

    std::cout << "[INFO]: Using reserved memory for " << ReservedMemoryManager::metadata[page].capacity() << " elements. [OK]" << std::endl;
    // Initialize domain data based on template
    for (size_t domain_index = 0; domain_index < EXTLIB_NUM_DOMAINS; ++domain_index)
    {
        if (template_timestamp->domain_data[domain_index].arr_size == 0)
            continue;

        DomainMemoryPool map;
        map.arr_size = template_timestamp->domain_data[domain_index].arr_size;
        size_t total_elements = map.arr_size * ReservedMemoryManager::reserved_memory_size;

        // Allocate flat arrays for all timestamps
        map.sizeof_energy_per_counter = sizeof(double) * total_elements;
        map.sizeof_counter_value = sizeof(uint64_t) * total_elements;
        map.sizeof_counter_name = sizeof(char *) * map.arr_size;

        map.counter_value = (uint64_t *)calloc(total_elements, sizeof(uint64_t));
        map.energy_per_counter = (double *)calloc(total_elements, sizeof(double));
        map.counter_name = (char **)calloc(map.arr_size, sizeof(char *));

        if (!map.counter_value || !map.energy_per_counter || !map.counter_name)
        {
            std::cerr << "[ERROR]: Failed to allocate reserved memory for domain arrays\n";
            exit(EXIT_FAILURE);
        }

        // Copy counter names
        for (size_t j = 0; j < map.arr_size; j++)
        {
            map.counter_name[j] = (char *)calloc(EXTLIB_MAX_DOMAIN_NAME_LENGTH, sizeof(char));
            if (!map.counter_name[j])
            {
                std::cerr << "[ERROR]: Failed to allocate memory for counter_name string\n";
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
        uint64_t page_id = entry.first;
        auto &vec = entry.second;

        // Check for cleared slots first (higher priority)
        for (size_t i = 0; i < vec.size(); ++i)
        {
            if (vec[i].id == 0)
            {
                DEBUG_LOG << "Reusing slot: page-id=" << page_id << ", index=" << i << "\n";
                entry_length[page_id] += 1;
                DEBUG_LOG << "page-id:" << page_id << " vec size:" << entry_length[page_id] << "!!\n";
                if (empty_page_ids.find(page_id) != empty_page_ids.end())
                {
                    DEBUG_LOG << "Removing page_id:" << page_id << " from empty_page_ids.\n";
                    empty_page_ids.erase(page_id);
                }
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
        if (empty_page_ids.find(first_non_full_page) != empty_page_ids.end())
        {
            DEBUG_LOG << "Removing page_id:" << first_non_full_page << " from empty_page_ids.\n";
            empty_page_ids.erase(first_non_full_page);
        }
        DEBUG_LOG << "Appending to page-id: " << first_non_full_page << "\n";
        auto &vec = metadata[first_non_full_page];
        vec.emplace_back();
        entry_length[first_non_full_page] += 1;
        DEBUG_LOG << "page-id:" << first_non_full_page << " vec size:" << entry_length[first_non_full_page] << "!!\n";
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
    for (size_t domain_id = 0; domain_id < EXTLIB_NUM_DOMAINS; domain_id++)
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

    uint64_t page = timestamp->reserved_memory_page;

    DEBUG_LOG << "\t before removal: page-id: " << page << " length: " << entry_length[page] << "\n";

    // Check if we should remove the page
    entry_length.at(page) -= 1;
    clear_timestamp(timestamp);

    if (should_remove_page(page))
    {
        remove_page(page);
    }
    // page is not removed but it is empty.
    // add it to empty page set and check any page to remove.
    else if (entry_length.at(page) == 0)
    {
        empty_page_ids.insert(page);
        DEBUG_LOG << "Inserting page_id:" << page << " to empty_page_ids" << "\n";

        while (empty_page_ids.size() > EXTLIB_MAX_ALLOWED_EMPTY_PAGES)
        {
            auto it = empty_page_ids.begin();
            while (it != empty_page_ids.end() && *it == 0)
                ++it;

            if (it == empty_page_ids.end())
                break; // nothing else to remove

            uint64_t page_to_remove = *it;
            DEBUG_LOG << "Removing page_id:" << page_to_remove << " from empty_page_ids.\n";
            empty_page_ids.erase(it);
            remove_page(page_to_remove);
        }
    }
}

ReservedMemoryManager::DomainMemoryPool *ReservedMemoryManager::get_domain_map(uint64_t domain_id, uint64_t page)
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
    DEBUG_LOG << "\tclosing sequence\n";

    for (auto &domain_entry : domain_memory_pool)
    {
        cntr_names_cleared = false; // clear for each domain.
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
                        DEBUG_LOG << "\t\t------> cleared for domain:" << domain_entry.first << "\n";
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
    empty_page_ids.clear();
    ReservedMemoryManager::reserved_memory_size = 0;
    ReservedMemoryManager::is_enabled = false;
    getset_new_pageid(0);
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
uint64_t ReservedMemoryManager::getset_new_pageid(uint64_t page)
{
    static uint64_t new_page_id = 0;

    if (page == -1) // get next page-id
    {
        new_page_id++;
        DEBUG_LOG << "Creating new page-id: " << new_page_id << "\n";
        return new_page_id;
    }
    else // used for re-setting the counter.
    {
        new_page_id = page;
        DEBUG_LOG << "Setting new page-id: " << new_page_id << "\n";
        return new_page_id;
    }
}
ExtlibEnergyTimeStamp *ReservedMemoryManager::create_new_page()
{
    // size_t new_page_id = metadata.size();
    // if middle page is removed, matadata.size() would collide with existing page ids.
    // So we use a static counter to ensure unique page ids.
    size_t new_page_id = getset_new_pageid();

    auto &new_vec = metadata[new_page_id];
    new_vec.reserve(ReservedMemoryManager::reserved_memory_size);
    new_vec.emplace_back();
    entry_length[new_page_id] = 1;
    DEBUG_LOG << "page-id:" << new_page_id << " vec size:" << entry_length[new_page_id] << "!!\n";

    // Create domain maps for new page
    for (auto &&i : domain_memory_pool)
    {
        uint64_t domain_id = i.first;
        auto &domain_map = i.second;

        DomainMemoryPool map;
        map.arr_size = domain_map.at(0).arr_size; // get the first page's arr_size
        size_t total_elements = map.arr_size * ReservedMemoryManager::reserved_memory_size;
        DEBUG_LOG << "total_elements: " << total_elements << " for domain_id: " << domain_id << "\n";

        map.sizeof_energy_per_counter = sizeof(double) * total_elements;
        map.sizeof_counter_value = sizeof(uint64_t) * total_elements;
        map.sizeof_counter_name = sizeof(char *) * map.arr_size;

        map.counter_value = (uint64_t *)calloc(total_elements, sizeof(uint64_t));
        map.energy_per_counter = (double *)calloc(total_elements, sizeof(double));
        map.counter_name = domain_map.at(0).counter_name; // Share counter names with the first page.

        if (!map.counter_value || !map.energy_per_counter || !map.counter_name)
        {
            std::cerr << "[ERROR]: Failed to allocate reserved memory for domain arrays\n";
            exit(EXIT_FAILURE);
        }

        // domain_memory_pool[domain_id][new_page_id] = std::move(map);
        domain_memory_pool.at(domain_id)[new_page_id] = std::move(map);
    }

    return initialize_timestamp(&new_vec.back(), new_page_id, 0);
}

bool ReservedMemoryManager::should_remove_page(uint64_t page)
{
    // if it is not the default page (page 0)
    // if there is no remaining entry in the page
    // if previous pages has empty entry slots (for optimization) > than the threshold.
    // remove the page.
    return page != 0 &&
           entry_length.at(page) == 0 &&
           count_total_empty_slots_before(page) >= EXTLIB_EMPTY_SLOT_THRESHOLD;
}

void ReservedMemoryManager::remove_page(uint64_t page)
{
    if (OPT_UNLIKELY(page == 0))
    {
        DEBUG_LOG << "Page-ID:0 cannot be removed.\n";
        return;
    }

    DEBUG_LOG << "FREEING PAGE-ID:" << page << "\n";
    metadata.erase(page);
    DEBUG_LOG << "----------->>>> Entry length for page-id " << page << " is:" << entry_length.at(page) << ".\n";
    entry_length.erase(page);

    // Free domain memory pool for this page
    for (size_t domain_id = 0; domain_id < EXTLIB_NUM_DOMAINS; domain_id++)
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
        DEBUG_LOG << "\t Domain: " << domain_id << " erased from page-id: " << page << "\n";
    }
}

void ReservedMemoryManager::shrink_to_fit()
{
    DEBUG_LOG << "\tshrink_to_fit called\n";

    while (true)
    {
        auto it = empty_page_ids.begin();
        while (it != empty_page_ids.end() && *it == 0)
            ++it;

        if (it == empty_page_ids.end())
            break; // nothing else to remove

        uint64_t page_to_remove = *it;
        DEBUG_LOG << "Removing page_id:" << page_to_remove << " from empty_page_ids.\n";
        empty_page_ids.erase(it);
        remove_page(page_to_remove);
    }
}

void ReservedMemoryManager::clear_timestamp(ExtlibEnergyTimeStamp *timestamp)
{
    DEBUG_LOG << "\t\tclear timestamp at index " << timestamp->reserved_memory_index
              << " from page: " << timestamp->reserved_memory_page << "\n";
    memset(timestamp, 0, sizeof(ExtlibEnergyTimeStamp));
    timestamp->id = 0;
    timestamp->reserved_memory_index = -1;
    timestamp->reserved_memory_page = -1;
}
