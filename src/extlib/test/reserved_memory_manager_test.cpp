// =============================================================================
// Memory Manager Tests
//
// Author: Osman Yasal <osman.yasal@vsb.cz>
// Version: 1.0.0
// License: BSD-3
// =============================================================================

#include <gtest/gtest.h>
#include "../meric_ext.h"
#include "../reserved_memory_manager.h"

#define RESERVED_ELEMENT_SIZE 12
TEST(MemoryManagerTest, ReservedMemoryManagerEnabledAndSizeIsCorrect)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
    EXTLIB_ENERGY_ENABLE_ALL_DOMAINS(energy_domains);
    extlib_init(&energy_domains, true);

    EXPECT_TRUE(ReservedMemoryManager::is_enabled);
    EXPECT_EQ(ReservedMemoryManager::reserved_memory_size, 3 * RESERVED_ELEMENT_SIZE);

    extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, InitializeCreatesPage0ForAllDomainsWithCorrectSize)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_ENABLE_ALL_DOMAINS(energy_domains);
    extlib_init(&energy_domains, true);
    ExtlibEnergyTimeStamp *reference_ts = extlib_read_energy_measurements(&energy_domains); // get a dummy timestamp

    // get existing domain sizes, to compare with memory manager allocations
    size_t domain_arr_size[EXTLIB_NUM_DOMAINS] = {0}; // init each domain arr size to 0
    for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
        {
            int domain_index = meric::mutils::countTrailingZeros(domain);
            domain_arr_size[domain_index] = reference_ts->domain_data[domain_index].arr_size;
            std::cout << "Domain Index: " << domain_index << ", Array Size: " << domain_arr_size[domain_index] << "\n";
        }
    }
    extlib_free_energy_timestamp(reference_ts);
    extlib_close(&energy_domains);

    energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
    EXTLIB_ENERGY_ENABLE_ALL_DOMAINS(energy_domains);
    extlib_init(&energy_domains, true);

    ExtlibEnergyTimeStamp *ts1 = extlib_read_energy_measurements(&energy_domains); // get a dummy timestamp from reserved memory

    for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
        {
            int domain_index = meric::mutils::countTrailingZeros(domain);
            ReservedMemoryManager::DomainMemoryPool *domain_memory_map = ReservedMemoryManager::get_domain_map(domain_index, 0); // to ensure page 0 is created for all domains.
            if (!domain_memory_map)
                continue;

            EXPECT_EQ(domain_memory_map->arr_size, domain_arr_size[domain_index]);

            EXPECT_NE(domain_memory_map->counter_name, nullptr);
            EXPECT_NE(domain_memory_map->counter_value, nullptr);
            EXPECT_NE(domain_memory_map->energy_per_counter, nullptr);

            EXPECT_EQ(domain_memory_map->sizeof_counter_name, domain_arr_size[domain_index] * sizeof(char *));
            EXPECT_EQ(domain_memory_map->sizeof_counter_value, sizeof(uint64_t) * domain_arr_size[domain_index] * ReservedMemoryManager::reserved_memory_size);
            EXPECT_EQ(domain_memory_map->sizeof_energy_per_counter, sizeof(double) * domain_arr_size[domain_index] * ReservedMemoryManager::reserved_memory_size);
        }
    }

    // log with gtest
    std::cout << "Filling page 0 to create another page!\n";
    for (int i = 0; i < RESERVED_ELEMENT_SIZE; i++)
    {
        auto *ts1 = extlib_read_energy_measurements(&energy_domains);
        auto *ts2 = extlib_read_energy_measurements(&energy_domains);
        auto *result = extlib_calc_energy_consumption(ts1, ts2);
        // don't free!
    }
    // page 1 must be created.
    ASSERT_TRUE(ReservedMemoryManager::entry_length.find(1) != ReservedMemoryManager::entry_length.end()) << "Page 1 not found in entry_length map";

    for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
        {
            int domain_index = meric::mutils::countTrailingZeros(domain);
            ReservedMemoryManager::DomainMemoryPool *domain_memory_map = ReservedMemoryManager::get_domain_map(domain_index, 1); // to ensure page 1 is created for all domains.
            if (!domain_memory_map)
                continue;

            EXPECT_EQ(domain_memory_map->arr_size, domain_arr_size[domain_index]);

            EXPECT_NE(domain_memory_map->counter_name, nullptr);
            EXPECT_NE(domain_memory_map->counter_value, nullptr);
            EXPECT_NE(domain_memory_map->energy_per_counter, nullptr);

            EXPECT_EQ(domain_memory_map->sizeof_counter_name, domain_arr_size[domain_index] * sizeof(char *));
            EXPECT_EQ(domain_memory_map->sizeof_counter_value, sizeof(uint64_t) * domain_arr_size[domain_index] * ReservedMemoryManager::reserved_memory_size);
            EXPECT_EQ(domain_memory_map->sizeof_energy_per_counter, sizeof(double) * domain_arr_size[domain_index] * ReservedMemoryManager::reserved_memory_size);
        }
    }

    extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, CloseFreesAllPagesAndLeavesManagerInResetState)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
    EXTLIB_ENERGY_ENABLE_ALL_DOMAINS(energy_domains);
    extlib_init(&energy_domains, true);
    extlib_close(&energy_domains);

    EXPECT_FALSE(ReservedMemoryManager::is_enabled);
    EXPECT_EQ(ReservedMemoryManager::reserved_memory_size, 0);
    EXPECT_EQ(ReservedMemoryManager::domain_memory_pool.size(), 0);
    EXPECT_EQ(ReservedMemoryManager::metadata.size(), 0);
    EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 0);
    EXPECT_EQ(ReservedMemoryManager::empty_page_ids.size(), 0);
}

TEST(MemoryManagerTest, MultipleInitCloseHasNoSideEffect)
{

    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
    EXTLIB_ENERGY_ENABLE_ALL_DOMAINS(energy_domains);
    extlib_init(&energy_domains, true);
    extlib_close(&energy_domains);

    extlib_init(&energy_domains, true);
    extlib_close(&energy_domains);

    extlib_init(&energy_domains, true);
    extlib_close(&energy_domains);

    // make sure reserved memory manager is clean state.
    EXPECT_FALSE(ReservedMemoryManager::is_enabled);
    EXPECT_EQ(ReservedMemoryManager::reserved_memory_size, 0);
    EXPECT_EQ(ReservedMemoryManager::domain_memory_pool.size(), 0);
    EXPECT_EQ(ReservedMemoryManager::metadata.size(), 0);
    EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 0);
    EXPECT_EQ(ReservedMemoryManager::empty_page_ids.size(), 0);

    // traverse each byte of energy_domains to ensure it is zeroed out.
    for (size_t i = 0; i < sizeof(ExtlibEnergy); i++)
    {
        EXPECT_EQ(*((char *)&energy_domains + i), 0) << "Non-zero byte found at position " << i;
    }
}

TEST(MemoryManagerTest, AllocateTimeStampPrioritizeUsingEmptySlots)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, 1);
    EXTLIB_ENERGY_ENABLE_ALL_DOMAINS(energy_domains);
    extlib_init(&energy_domains, true);

    // page 0 allocations
    ExtlibEnergyTimeStamp *ts1 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *ts2 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *result = extlib_calc_energy_consumption(ts1, ts2);

    // goes to page 1 due to size limit (1)
    ExtlibEnergyTimeStamp *ts3 = extlib_read_energy_measurements(&energy_domains);

    EXPECT_EQ(ts1->reserved_memory_page, 0);
    EXPECT_EQ(ts1->reserved_memory_index, 0);

    EXPECT_EQ(ts2->reserved_memory_page, 0);
    EXPECT_EQ(ts2->reserved_memory_index, 1);

    EXPECT_EQ(result->reserved_memory_page, 0);
    EXPECT_EQ(result->reserved_memory_index, 2);

    // t3 is allocated in new page.
    EXPECT_EQ(ts3->reserved_memory_page, 1);
    EXPECT_EQ(ts3->reserved_memory_index, 0);

    // freeing one slot in page 0, creates available slot. next allocation should go there.
    // this should allocate on page 0 index 2
    extlib_free_energy_timestamp(result);
    ts1 = extlib_read_energy_measurements(&energy_domains);
    EXPECT_EQ(ts1->reserved_memory_page, 0);
    EXPECT_EQ(ts1->reserved_memory_index, 2);

    // freeing one slot in page 0, creates available slot. next allocation should go there.
    // this should allocate on page 0 index 1
    extlib_free_energy_timestamp(ts2);
    ts2 = extlib_read_energy_measurements(&energy_domains);
    EXPECT_EQ(ts2->reserved_memory_page, 0);
    EXPECT_EQ(ts2->reserved_memory_index, 1);

    extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, AllocateTimeStampCreatesNewPageWhenPageIsFull)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, 1);
    EXTLIB_ENERGY_ENABLE_ALL_DOMAINS(energy_domains);
    EXTLIB_ENERGY_DISABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON);
    extlib_init(&energy_domains, true);

    // page 0 allocations
    ExtlibEnergyTimeStamp *ts1 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *ts2 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *result = extlib_calc_energy_consumption(ts1, ts2);

    // goes to page 1 due to size limit (1)
    ExtlibEnergyTimeStamp *ts3 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *ts4 = extlib_read_energy_measurements(&energy_domains);
    ExtlibEnergyTimeStamp *result2 = extlib_read_energy_measurements(&energy_domains);

    EXPECT_EQ(ts3->reserved_memory_page, 1);
    EXPECT_EQ(ts3->reserved_memory_index, 0);
    EXPECT_EQ(ts4->reserved_memory_page, 1);
    EXPECT_EQ(ts4->reserved_memory_index, 1);
    EXPECT_EQ(result2->reserved_memory_page, 1);
    EXPECT_EQ(result2->reserved_memory_index, 2);

    EXPECT_TRUE(ReservedMemoryManager::metadata.find(0) != ReservedMemoryManager::metadata.end());
    EXPECT_TRUE(ReservedMemoryManager::metadata.find(1) != ReservedMemoryManager::metadata.end());

    EXPECT_EQ(ReservedMemoryManager::entry_length.at(0), 3);
    EXPECT_EQ(ReservedMemoryManager::entry_length.at(1), 3);

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
    // Test body to be implemented
}

TEST(MemoryManagerTest, PageIsRemovedWhenConditionIsMet)
{
    // Test body to be implemented
}

TEST(MemoryManagerTest, PageIsNotRemovedIfConditionNotMet)
{
    // Test body to be implemented
}

TEST(MemoryManagerTest, Page0IsNeverRemovedUnlessCloseSequenceIsCalled)
{

    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, 1);
    EXTLIB_ENERGY_ENABLE_ALL_DOMAINS(energy_domains);
    extlib_init(&energy_domains, true);

    EXPECT_EQ(ReservedMemoryManager::empty_page_ids.count(0), 1);
    EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.find(0) != ReservedMemoryManager::empty_page_ids.end());

    EXPECT_EQ(ReservedMemoryManager::metadata.count(0), 1);

    EXPECT_EQ(ReservedMemoryManager::entry_length.count(0), 1);
    EXPECT_EQ(ReservedMemoryManager::entry_length.at(0), 0);

    ReservedMemoryManager::shrink_to_fit();

    EXPECT_EQ(ReservedMemoryManager::empty_page_ids.count(0), 1);
    EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.find(0) != ReservedMemoryManager::empty_page_ids.end());
    EXPECT_EQ(ReservedMemoryManager::metadata.count(0), 1);
    EXPECT_EQ(ReservedMemoryManager::entry_length.count(0), 1);
    EXPECT_EQ(ReservedMemoryManager::entry_length.at(0), 0);

    ReservedMemoryManager::remove_page(0);

    EXPECT_EQ(ReservedMemoryManager::empty_page_ids.count(0), 1);
    EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.find(0) != ReservedMemoryManager::empty_page_ids.end());
    EXPECT_EQ(ReservedMemoryManager::metadata.count(0), 1);
    EXPECT_EQ(ReservedMemoryManager::entry_length.count(0), 1);
    EXPECT_EQ(ReservedMemoryManager::entry_length.at(0), 0);

    extlib_close(&energy_domains);

    EXPECT_EQ(ReservedMemoryManager::empty_page_ids.count(0), 0);
    EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.find(0) == ReservedMemoryManager::empty_page_ids.end());
    EXPECT_EQ(ReservedMemoryManager::metadata.count(0), 0);
    EXPECT_EQ(ReservedMemoryManager::entry_length.count(0), 0);
}

// =============================================================================
// State Consistency Tests
// =============================================================================

TEST(MemoryManagerTest, MetadataConsistencyAfterPageRemoval)
{
    // Test body to be implemented
}

TEST(MemoryManagerTest, EntryLengthTrackingAccuracyAcrossOperations)
{
    // Test body to be implemented
}

TEST(MemoryManagerTest, EmptyPageSetConsistency)
{
    // Test body to be implemented
}

TEST(MemoryManagerTest, CounterNameSharingAcrossPagesInSameDomain)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, 1);
    EXTLIB_ENERGY_ENABLE_ALL_DOMAINS(energy_domains);
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
            std::cout << "Domain ID: " << domain_id << ",ts1 Counter Name Address: " << static_cast<void *>(ts1->domain_data[domain_id].counter_name) << "\n";
            std::cout << "Domain ID: " << domain_id << ",ts2 Counter Name Address: " << static_cast<void *>(ts2->domain_data[domain_id].counter_name) << "\n";
            std::cout << "Domain ID: " << domain_id << ",ts3 Counter Name Address: " << static_cast<void *>(ts3->domain_data[domain_id].counter_name) << "\n";
            std::cout << "Domain ID: " << domain_id << ",ts4 Counter Name Address: " << static_cast<void *>(ts4->domain_data[domain_id].counter_name) << "\n";
            std::cout << "--------------\n";
        }
    }
    extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, OffsetCalculationCorrectnessForDifferentArraySizes)
{
    // Test body to be implemented
}