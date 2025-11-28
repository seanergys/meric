// =============================================================================
// Memory Manager Tests
//
// Author: Osman Yasal <osman.yasal@vsb.cz>
// Version: 1.0.0
// License: BSD-3
// =============================================================================

#include <string>

#include <meric_ext.h>
#include <gtest/gtest.h>

#include "../reserved_memory_manager.h"
#include "work.h"

void enableAllEnergyDomains(ExtlibEnergy& energy_domains)
{
        EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX);
        EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL);
        EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML);
        EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM);
        EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM);
}

std::string getEnergyDomainName(unsigned int domain)
{
    std::string domain_name;
    switch (domain)
    {
        case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX:
            domain_name = "A64FX";
            break;
        case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL:
            domain_name = "RAPL";
            break;
        case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML:
            domain_name = "NVML";
            break;
        case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM:
            domain_name = "ROCM";
            break;
        case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM:
            domain_name = "HDEEM";
            break;
        case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON:
            domain_name = "HWMON";
            break;
        default:
            domain_name = "Unknown Domain";
            break;
    }

    return domain_name;
}

#define RESERVED_ELEMENT_SIZE 12

TEST(MemoryManagerTest, ReservedMemoryManagerIsEnabledAndSizeOfReservedMemoryIsCorrect)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
    size_t expected_reserved_memory_size = 2 * RESERVED_ELEMENT_SIZE;

    EXPECT_TRUE(energy_domains.use_reserved_memory)
        << "use_reserved_memory boolean should be true.";
    EXPECT_EQ(energy_domains.reserved_memory_size, expected_reserved_memory_size)
        << "reserved_memory_size variable should be " << expected_reserved_memory_size
        << " (" << energy_domains.reserved_memory_size << " != " << expected_reserved_memory_size << ").";

    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    EXPECT_TRUE(ReservedMemoryManager::is_enabled)
        << "is_enabled boolean should be true.";
    EXPECT_EQ(ReservedMemoryManager::reserved_memory_size, expected_reserved_memory_size)
        << "reserved_memory_size variable should be " << expected_reserved_memory_size
        << " (" << ReservedMemoryManager::reserved_memory_size << " != " << expected_reserved_memory_size << ").";

    extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, ReservedMemoryPage0IsCreatedForExistingEnergyDomainsAfterInit)
{
    // note that since the index0 of page0 is reserved to save result, we have +1 in size.
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    ExtlibEnergyTimeStamp *reference_ts = extlib_read_energy_measurements(&energy_domains); // get a dummy timestamp
    // get existing domain sizes, to compare with memory manager allocations
    size_t domain_arr_size[EXTLIB_NUM_DOMAINS] = {0}; // init each domain arr size to 0
    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
        {
            domain_arr_size[domain_idx] = reference_ts->domain_data[domain_idx].arr_size;
        }
    }

    extlib_free_energy_timestamp(reference_ts);
    extlib_close(&energy_domains);

    energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.empty())
        << "empty_page_ids container should be empty.";

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
        {
            ReservedMemoryManager::DomainMemoryPool *domain_memory_map = ReservedMemoryManager::get_domain_map(domain_idx, 0); // to ensure page 0 is created for all domains.

            std::string domain_name = getEnergyDomainName(domain);
            if (!domain_memory_map)
            {
                ADD_FAILURE() << domain_name << " energy domain exists but pointer to its reserved memory page 0 is NULL/nullptr.";
                continue;
            }
            EXPECT_NE(domain_memory_map->counter_name, nullptr)
                << domain_name << " energy domain exists but counter_name pointer of its reserved memory page 0 is NULL/nullptr.";
            EXPECT_NE(domain_memory_map->counter_value, nullptr)
                << domain_name << " energy domain exists but counter_value pointer of its reserved memory page 0 is NULL/nullptr.";
            EXPECT_NE(domain_memory_map->energy_per_counter, nullptr)
                << domain_name << " energy domain exists but energy_per_counter pointer of its reserved memory page 0 is NULL/nullptr.";

            size_t expected_arr_size = domain_arr_size[domain_idx];
            size_t expected_sizeof_counter_name = sizeof(char *) * domain_arr_size[domain_idx];
            size_t expected_sizeof_counter_value = sizeof(uint64_t) * domain_arr_size[domain_idx] * (ReservedMemoryManager::reserved_memory_size + 1); // +1 for result timestamp
            size_t expected_sizeof_energy_per_counter = sizeof(double) * domain_arr_size[domain_idx] * (ReservedMemoryManager::reserved_memory_size + 1);

            EXPECT_EQ(domain_memory_map->arr_size, expected_arr_size)
                << "arr_size variable of reserved memory page 0 for existing " << domain_name << " energy domain should be " << expected_arr_size
                << " (" << domain_memory_map->arr_size << " != " << expected_arr_size << ").";
            EXPECT_EQ(domain_memory_map->sizeof_counter_name, expected_sizeof_counter_name)
                << "sizeof_counter_name variable of reserved memory page 0 for existing " << domain_name << " energy domain should be " << expected_sizeof_counter_name
                << " (" << domain_memory_map->sizeof_counter_name << " != " << expected_sizeof_counter_name << ").";
            EXPECT_EQ(domain_memory_map->sizeof_counter_value, expected_sizeof_counter_value)
                << "sizeof_counter_value variable of reserved memory page 0 for existing " << domain_name << " energy domain should be " << expected_sizeof_counter_value
                << " (" << domain_memory_map->sizeof_counter_value << " != " << expected_sizeof_counter_value << ").";
            EXPECT_EQ(domain_memory_map->sizeof_energy_per_counter, expected_sizeof_energy_per_counter)
                << "sizeof_energy_per_counter variable of reserved memory page 0 for existing " << domain_name << " energy domain should be " << expected_sizeof_energy_per_counter
                << " (" << domain_memory_map->sizeof_energy_per_counter << " != " << expected_sizeof_energy_per_counter << ").";
        }
    }

    extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, ReservedMemoryPage0IsNotCreatedForEnabledNonExistingEnergyDomainsAfterInit)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
        {
            ReservedMemoryManager::DomainMemoryPool *domain_memory_map = ReservedMemoryManager::get_domain_map(domain_idx, 0);

            std::string domain_name = getEnergyDomainName(domain);
            EXPECT_EQ(domain_memory_map, nullptr)
                << domain_name << " energy domain does not exist but its reserved memory page 0 is not NULL/nullptr.";
        }
    }

    extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, ReservedMemoryPage0IsNotCreatedForDisabledEnergyDomainsAfterInit)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
    EXTLIB_ENERGY_NO_SELECTION(energy_domains);
    extlib_init(&energy_domains, true);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        ReservedMemoryManager::DomainMemoryPool *domain_memory_map = ReservedMemoryManager::get_domain_map(domain_idx, 0);

        std::string domain_name = getEnergyDomainName(domain);
        EXPECT_EQ(domain_memory_map, nullptr)
            << domain_name << " energy domain is disabled but its reserved memory page 0 is not NULL/nullptr.";

    }
}

TEST(MemoryManagerTest, ReservedMemoryManagerIsInResetStateAfterClose)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    int n_ts = (RESERVED_ELEMENT_SIZE*2) + 1;
    for (int i = 0; i < n_ts; i++)
    {
        auto *ts = extlib_read_energy_measurements(&energy_domains);
    }

    extlib_close(&energy_domains);

    EXPECT_FALSE(ReservedMemoryManager::is_enabled)
        << "is_enabled boolean should be false after close.";
    EXPECT_EQ(ReservedMemoryManager::reserved_memory_size, 0)
        << "reserved_memory_size variable should be false after close.";
    EXPECT_EQ(ReservedMemoryManager::domain_memory_pool.size(), 0)
        << "size of domain_memory_pool container should be 0 after close.";
    EXPECT_EQ(ReservedMemoryManager::metadata.size(), 0)
        << "size of metadata container should be 0 after close.";
    EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 0)
        << "size of entry_length container should be 0 after close.";
    EXPECT_EQ(ReservedMemoryManager::empty_page_ids.size(), 0)
        << "size of empty_page_ids container should be 0 after close.";
}

TEST(MemoryManagerTest, MultipleInitAndCloseHaveNoSideEffects)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);
    extlib_close(&energy_domains);

    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);
    extlib_close(&energy_domains);

    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);
    extlib_close(&energy_domains);

    // make sure ReservedMemoryManager is in reset state.
    EXPECT_FALSE(ReservedMemoryManager::is_enabled)
        << "is_enabled boolean should be false after close.";
    EXPECT_EQ(ReservedMemoryManager::reserved_memory_size, 0)
        << "reserved_memory_size variable should be false after close.";
    EXPECT_EQ(ReservedMemoryManager::domain_memory_pool.size(), 0)
        << "size of domain_memory_pool container should be 0 after close.";
    EXPECT_EQ(ReservedMemoryManager::metadata.size(), 0)
        << "size of metadata container should be 0 after close.";
    EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 0)
        << "size of entry_length container should be 0 after close.";
    EXPECT_EQ(ReservedMemoryManager::empty_page_ids.size(), 0)
        << "size of empty_page_ids container should be 0 after close.";

    // make sure energy_domains is in reset state.
    EXPECT_EQ(energy_domains.value, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NO_SELECTION)
        << "All energy domains should be disabled after close.";
    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        std::string domain_name = getEnergyDomainName(domain);
        EXPECT_EQ(energy_domains.energyMeasurementFns[domain_idx], nullptr)
            << "Energy measurement function of " << domain_name << " energy domain should not be set after close.";
        EXPECT_EQ(energy_domains.closeFns[domain_idx], nullptr)
            << "Close function of " << domain_name << " energy domain should not be set after close.";
    }
    EXPECT_FALSE(energy_domains.use_reserved_memory)
        << "use_reserved_memory boolean should be false after close.";
    EXPECT_EQ(energy_domains.reserved_memory_size, 0)
        << "reserved_memory_size variable should be 0 after close.";
}

TEST(MemoryManagerTest, AllocateTimeStampPrioritizeUsingEmptySlots)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, 1);
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    // page 0 allocations
    ExtlibEnergyTimeStamp *ts1 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *ts2 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *result = extlib_calc_energy_consumption(ts1, ts2);

    // goes to page 1 due to size limit (1)
    ExtlibEnergyTimeStamp *ts3 = extlib_read_energy_measurements(&energy_domains);

    EXPECT_EQ(ts1->reserved_memory_page, 0);
    EXPECT_EQ(ts1->reserved_memory_index, 1);

    EXPECT_EQ(ts2->reserved_memory_page, 0);
    EXPECT_EQ(ts2->reserved_memory_index, 2);

    EXPECT_EQ(result->reserved_memory_page, 0);
    EXPECT_EQ(result->reserved_memory_index, 0); // result is always at page 0 index 0

    // t3 is allocated in new page.
    EXPECT_EQ(ts3->reserved_memory_page, 1);
    EXPECT_EQ(ts3->reserved_memory_index, 0);

    // freeing one slot in page 0, creates available slot. next allocation should go there.
    // this should allocate on page 0 index 2
    extlib_free_energy_timestamp(result);
    ts1 = extlib_read_energy_measurements(&energy_domains);
    EXPECT_EQ(ts1->reserved_memory_page, 1);
    EXPECT_EQ(ts1->reserved_memory_index, 1);

    // freeing one slot in page 0, creates available slot. next allocation should go there.
    // this should allocate on page 0 index 1
    extlib_free_energy_timestamp(ts2);
    ts2 = extlib_read_energy_measurements(&energy_domains);
    EXPECT_EQ(ts2->reserved_memory_page, 0);
    EXPECT_EQ(ts2->reserved_memory_index, 2);

    extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, AllocateTimeStampCreatesNewPageWhenPageIsFull)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, 1);
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    // page 0 allocations
    ExtlibEnergyTimeStamp *ts1 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *ts2 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *result = extlib_calc_energy_consumption(ts1, ts2);

    EXPECT_EQ(ts1->reserved_memory_page, 0);
    EXPECT_EQ(ts1->reserved_memory_index, 1);
    EXPECT_EQ(ts2->reserved_memory_page, 0);
    EXPECT_EQ(ts2->reserved_memory_index, 2);
    EXPECT_EQ(result->reserved_memory_page, 0);
    EXPECT_EQ(result->reserved_memory_index, 0);

    std::cout << "-------------------------\n";
    // goes to page 1 due to size limit (1)
    ExtlibEnergyTimeStamp *ts3 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *ts4 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *result2 = extlib_calc_energy_consumption(ts3, ts4);

    EXPECT_EQ(ts3->reserved_memory_page, 1);
    EXPECT_EQ(ts3->reserved_memory_index, 0);
    EXPECT_EQ(ts4->reserved_memory_page, 1);
    EXPECT_EQ(ts4->reserved_memory_index, 1);
    EXPECT_EQ(result2->reserved_memory_page, 0);
    EXPECT_EQ(result2->reserved_memory_index, 0);

    EXPECT_TRUE(ReservedMemoryManager::metadata.find(0) != ReservedMemoryManager::metadata.end());
    EXPECT_TRUE(ReservedMemoryManager::metadata.find(1) != ReservedMemoryManager::metadata.end());

    EXPECT_EQ(ReservedMemoryManager::entry_length.at(0), 3);
    EXPECT_EQ(ReservedMemoryManager::entry_length.at(1), 2);

    for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
        {
            int domain_id = meric::mutils::countTrailingZeros(domain);
            std::cout << "domain-id:" << domain_id << "\n";
            EXPECT_TRUE(ReservedMemoryManager::domain_memory_pool.find(domain_id) != ReservedMemoryManager::domain_memory_pool.end());
        }
    }
    extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, DeallocateClearsCorrespondingMemorySlotAndMarksAsEmpty)
{
    // init for all domains
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, 1);
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    // make measurement for 5sec job and save pointers.
    ExtlibEnergyTimeStamp *ts_array[6];
    ExtlibEnergyTimeStamp *ts1 = extlib_read_energy_measurements(&energy_domains);
    extlib_print_energy_timestamp(ts1, -1);

    do_work(5.0);

    ExtlibEnergyTimeStamp *ts2 = extlib_read_energy_measurements(&energy_domains);
    extlib_print_energy_timestamp(ts2, -1);

    ExtlibEnergyTimeStamp *result = extlib_calc_energy_consumption(ts1, ts2);
    extlib_print_energy_timestamp(result, -1);

    ts_array[0] = ts1;
    ts_array[1] = ts2;
    ts_array[2] = result;

    ts1 = extlib_read_energy_measurements(&energy_domains);
    extlib_print_energy_timestamp(ts1, -1);

    do_work(5.0);

    ts2 = extlib_read_energy_measurements(&energy_domains);
    extlib_print_energy_timestamp(ts2, -1);

    result = extlib_calc_energy_consumption(ts1, ts2);
    extlib_print_energy_timestamp(result, -1);

    ts_array[3] = ts1;
    ts_array[4] = ts2;
    ts_array[5] = result;

    // for each timestamp;
    // ->> first save the adresses energy_ts pointing to
    // ->> then free the timestamp
    // ->> then check if the memory locations are cleared (zeroed out)
    for (int i = 0; i < 6; ++i)
    {
        size_t domain_arr_size[EXTLIB_NUM_DOMAINS] = {};
        double *energy_per_counter[EXTLIB_NUM_DOMAINS] = {};
        uint64_t *counter_value[EXTLIB_NUM_DOMAINS] = {};
        size_t reserved_memory_page = ts_array[i]->reserved_memory_page;
        size_t reserved_memory_index = ts_array[i]->reserved_memory_index;

        std::cout << "-------------------\n";
        std::cout << "Timestamp ID:" << ts_array[i]->id << "\n";
        std::cout << "Checking timestamp at page:" << reserved_memory_page << " index:" << reserved_memory_index << "\n";

        for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
        {
            if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
            {
                int domain_id = meric::mutils::countTrailingZeros(domain);
                std::cout << "domain-id:" << domain_id << "\n";
                domain_arr_size[domain_id] = ts_array[i]->domain_data[domain_id].arr_size;
                energy_per_counter[domain_id] = ts_array[i]->domain_data[domain_id].energy_per_counter; // get the reference to the pre-allocated memory
                counter_value[domain_id] = ts_array[i]->domain_data[domain_id].counter_value;           // get the reference to the pre-allocated memory
                std::cout << "start index of counter_value[" << domain_id << "]:" << counter_value[domain_id] << "\n";
                std::cout << "start index of energy_per_counter[" << domain_id << "]:" << energy_per_counter[domain_id] << "\n";
            }
        }

        extlib_free_energy_timestamp(ts_array[i]); // now free the timestamp, we expect those fields to be zero
        for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
        {
            if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
            {
                int domain_id = meric::mutils::countTrailingZeros(domain);
                size_t offset = reserved_memory_index * domain_arr_size[domain_id];
                ReservedMemoryManager::DomainMemoryPool *domain_memory_map = ReservedMemoryManager::get_domain_map(domain_id, reserved_memory_page);
                if (!domain_memory_map)
                {
                    std::cout << "Requested page:" << reserved_memory_page << " for domain:" << domain_id << " not found!\n";
                    continue;
                }
                for (int i = 0; i < domain_arr_size[domain_id]; i++)
                {
                    if (i == 0)
                    {
                        std::cout << "Checking start index of counter_value[" << domain_id << "]:" << counter_value[domain_id] << "\n";
                        std::cout << "Checking start index of energy_per_counter[" << domain_id << "]:" << energy_per_counter[domain_id] << "\n";

                        EXPECT_EQ(counter_value[domain_id], &domain_memory_map->counter_value[offset + i]);
                        EXPECT_EQ(energy_per_counter[domain_id], &domain_memory_map->energy_per_counter[offset + i]);
                    }

                    EXPECT_EQ(domain_memory_map->energy_per_counter[offset + i], 0.0);
                    EXPECT_EQ(domain_memory_map->counter_value[offset + i], 0);

                    std::cout << "Checking counter_value to be 0 for domain[" << domain_id << "]:" << (counter_value[domain_id] + i) << "\n";
                    std::cout << "Checking energy_per_counter to be 0 for domain[" << domain_id << "]:" << (energy_per_counter[domain_id] + i) << "\n";

                    EXPECT_EQ(*(counter_value[domain_id] + i), 0);
                    EXPECT_EQ(*(energy_per_counter[domain_id] + i), 0);
                }
            }
        }
    }

    // ADD_FAILURE() << "TODO: Check if the page is marked as empty in empty_page_ids set";
    extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, PageIsRemovedWhenConditionIsMet)
{

    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, 1);
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    // TODO: Check the should_remove_page logic to see how it works,and create that conditions and make sure page is removed.
    // perform 2 energy readings (in total 6 timestamps you have), then free the first ones in page0 then remove ones in page1.
    // you need to see that page1 is removed and page0 is not removed. (by default we never remove it unless it is close sequence.)

    extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, PageIsNotRemovedIfConditionNotMet)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, 1);
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    // TODO: Similar to the previous one except this time you remove all entries on page 1 and, leave page 0 as-is. and make sure page 1 is not removed.
    extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, Page0IsNeverRemovedUnlessCloseSequenceIsCalled)
{

    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, 1);
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    EXPECT_EQ(ReservedMemoryManager::empty_page_ids.count(0), 0); // index0 is reserved for result timestamps, so page0 is never empty.
    EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.find(0) == ReservedMemoryManager::empty_page_ids.end());

    EXPECT_EQ(ReservedMemoryManager::metadata.count(0), 1);

    EXPECT_EQ(ReservedMemoryManager::entry_length.count(0), 1);
    EXPECT_EQ(ReservedMemoryManager::entry_length.at(0), 1);

    ReservedMemoryManager::shrink_to_fit();

    EXPECT_EQ(ReservedMemoryManager::empty_page_ids.count(0), 0);
    EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.find(0) == ReservedMemoryManager::empty_page_ids.end());
    EXPECT_EQ(ReservedMemoryManager::metadata.count(0), 1);
    EXPECT_EQ(ReservedMemoryManager::entry_length.count(0), 1);
    EXPECT_EQ(ReservedMemoryManager::entry_length.at(0), 1);

    ReservedMemoryManager::remove_page(0);

    EXPECT_EQ(ReservedMemoryManager::empty_page_ids.count(0), 0);
    EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.find(0) == ReservedMemoryManager::empty_page_ids.end());
    EXPECT_EQ(ReservedMemoryManager::metadata.count(0), 1);
    EXPECT_EQ(ReservedMemoryManager::entry_length.count(0), 1);
    EXPECT_EQ(ReservedMemoryManager::entry_length.at(0), 1);

    extlib_close(&energy_domains);

    EXPECT_EQ(ReservedMemoryManager::empty_page_ids.count(0), 0);
    EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.find(0) == ReservedMemoryManager::empty_page_ids.end());
    EXPECT_EQ(ReservedMemoryManager::metadata.count(0), 0);
    EXPECT_EQ(ReservedMemoryManager::entry_length.count(0), 0);
}

// =============================================================================
// State Consistency Tests
// =============================================================================
TEST(MemoryManagerTest, EmptyPageConsistency)
{
    // TODO: After removing a page, none of the fields should have a page id that was removed.
    /*
        static std::unordered_map<uint64_t, std::unordered_map<uint64_t, DomainMemoryPool>> domain_memory_pool; // domain_id -> page -> memory_map
        static std::unordered_map<uint64_t, std::vector<ExtlibEnergyTimeStamp>> metadata;                       // page -> timestamp metadata
        static std::unordered_map<uint64_t, uint64_t> entry_length;                                             // page -> num of entries
        static std::unordered_set<uint64_t> empty_page_ids;                                                     // List of empty page IDs for quick access, note that page 0 is NOT added.
    */
}

TEST(MemoryManagerTest, CounterNameSharingAcrossPagesInSameDomain)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, 1);
    enableAllEnergyDomains(energy_domains);
    EXTLIB_ENERGY_DISABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON);
    extlib_init(&energy_domains, true);

    ExtlibEnergyTimeStamp *ts1 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *ts2 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *result1 = extlib_calc_energy_consumption(ts1, ts2);

    // Creates another page
    ExtlibEnergyTimeStamp *ts3 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *ts4 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *result2 = extlib_calc_energy_consumption(ts3, ts4);

    EXPECT_TRUE(ReservedMemoryManager::metadata.find(1) != ReservedMemoryManager::metadata.end());
    EXPECT_TRUE(ReservedMemoryManager::entry_length.find(1) != ReservedMemoryManager::entry_length.end());

    for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
        {
            int domain_id = meric::mutils::countTrailingZeros(domain);
            EXPECT_EQ(ts1->domain_data[domain_id].counter_name, ts2->domain_data[domain_id].counter_name);
            EXPECT_EQ(ts1->domain_data[domain_id].counter_name, ts3->domain_data[domain_id].counter_name);
            EXPECT_EQ(ts1->domain_data[domain_id].counter_name, ts4->domain_data[domain_id].counter_name);
            EXPECT_EQ(ts2->domain_data[domain_id].counter_name, ts3->domain_data[domain_id].counter_name);
            EXPECT_EQ(ts2->domain_data[domain_id].counter_name, ts4->domain_data[domain_id].counter_name);

            // print their adresses for all
            std::cout << std::hex << "Domain ID: " << domain_id << ",ts1 Counter Name Address: " << (ts1->domain_data[domain_id].counter_name) << "\n";
            std::cout << std::hex << "Domain ID: " << domain_id << ",ts2 Counter Name Address: " << (ts2->domain_data[domain_id].counter_name) << "\n";
            std::cout << std::hex << "Domain ID: " << domain_id << ",ts3 Counter Name Address: " << (ts3->domain_data[domain_id].counter_name) << "\n";
            std::cout << std::hex << "Domain ID: " << domain_id << ",ts4 Counter Name Address: " << (ts4->domain_data[domain_id].counter_name) << "\n";
            std::cout << std::hex << "--------------\n";
        }
    }
    extlib_close(&energy_domains);
}
