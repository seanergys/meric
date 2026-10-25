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
	EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON);
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

	size_t expected_reserved_memory_size = RESERVED_ELEMENT_SIZE * 2;
	EXPECT_TRUE(energy_domains.use_reserved_memory) << "use_reserved_memory boolean should be true.";
	EXPECT_EQ(energy_domains.reserved_memory_size, expected_reserved_memory_size)
		<< "reserved_memory_size variable should be " << expected_reserved_memory_size << " (" << energy_domains.reserved_memory_size
		<< " != " << expected_reserved_memory_size << ").";

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
				FAIL() << domain_name << " energy domain exists but pointer to its reserved memory page 0 is NULL/nullptr.";
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
				<< "arr_size variable of reserved memory page 0 of existing " << domain_name << " energy domain should be " << expected_arr_size << " ("
				<< domain_memory_map->arr_size << " != " << expected_arr_size << ").";
			EXPECT_EQ(domain_memory_map->sizeof_counter_name, expected_sizeof_counter_name)
				<< "sizeof_counter_name variable of reserved memory page 0 of existing " << domain_name << " energy domain should be "
				<< expected_sizeof_counter_name << " (" << domain_memory_map->sizeof_counter_name << " != " << expected_sizeof_counter_name << ").";
			EXPECT_EQ(domain_memory_map->sizeof_counter_value, expected_sizeof_counter_value)
				<< "sizeof_counter_value variable of reserved memory page 0 of existing " << domain_name << " energy domain should be "
				<< expected_sizeof_counter_value << " (" << domain_memory_map->sizeof_counter_value << " != " << expected_sizeof_counter_value << ").";
			EXPECT_EQ(domain_memory_map->sizeof_energy_per_counter, expected_sizeof_energy_per_counter)
				<< "sizeof_energy_per_counter variable of reserved memory page 0 of existing " << domain_name << " energy domain should be "
				<< expected_sizeof_energy_per_counter << " (" << domain_memory_map->sizeof_energy_per_counter << " != " << expected_sizeof_energy_per_counter
				<< ").";
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

	extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, EnergyTimeStampMetadataForReservedMemoryPage0AreInitializedAfterInit)
{
	ExtlibEnergy energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 1) << "Size of metadata container should be 1.";
	ASSERT_EQ(ReservedMemoryManager::metadata.count(0), 1) << "Container for energy timestamp metadata for reserved memory page 0 is missing.";
	EXPECT_EQ(ReservedMemoryManager::metadata[0].capacity(), ReservedMemoryManager::reserved_memory_size + 1)
		<< "Capacity of container for energy timestamp metadata for reserved memory page 0 should be " << (ReservedMemoryManager::reserved_memory_size + 1)
		<< " (" << ReservedMemoryManager::metadata[0].capacity() << " != " << (ReservedMemoryManager::reserved_memory_size + 1) << ").";

	extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, ResultEnergyTimeStampIsInitializedAtIndex0OfReservedMemoryPage0AfterInit)
{
	ExtlibEnergy energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	EXPECT_EQ(ReservedMemoryManager::metadata[0].size(), 1) << "Size of container for energy timestamp metadata for reserved memory page 0 should be 1.";
	ExtlibEnergyTimeStamp *result_ts = &ReservedMemoryManager::metadata[0][0];

	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		std::string domain_name = getEnergyDomainName(domain);
		EXPECT_EQ(result_ts->domain_data[domain_idx].domain_id, domain)
			<< "ID of " << domain_name << " energy domain should be " << domain << " (" << (unsigned int)result_ts->domain_data[domain_idx].domain_id
			<< " != " << domain << ").";
		EXPECT_EQ(result_ts->domain_data[domain_idx].arr_size, 0) << "arr_size variable of " << domain_name << " energy domain should be 0.";
		EXPECT_EQ(result_ts->domain_data[domain_idx].counter_name, nullptr)
			<< "counter_name pointer of " << domain_name << " energy domain should be NULL/nullptr.";
		EXPECT_EQ(result_ts->domain_data[domain_idx].counter_value, nullptr)
			<< "counter_value pointer of " << domain_name << " energy domain should be NULL/nullptr.";
		EXPECT_EQ(result_ts->domain_data[domain_idx].energy_per_counter, nullptr)
			<< "energy_per_counter pointer of " << domain_name << " energy domain should be NULL/nullptr.";
		EXPECT_EQ(result_ts->domain_data[domain_idx].energy_total, 0.0) << "energy_total variable of " << domain_name << " energy domain should be 0.";
	}
	EXPECT_EQ(result_ts->energy_total, 0.0) << "energy_total variable should be 0.";
	EXPECT_EQ(result_ts->reserved_memory_page, 0) << "reserved_memory_page variable should be 0.";
	EXPECT_EQ(result_ts->reserved_memory_index, 0) << "reserved_memory_index variable should be 0.";

	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 1) << "Size of entry_length container should be 1.";
	ASSERT_EQ(ReservedMemoryManager::entry_length.count(0), 1) << "Number of entries in reserved memory page 0 is missing.";
	EXPECT_EQ(ReservedMemoryManager::entry_length[0], 1) << "Number of entries in reserved memory page 0 should be 1.";

	extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, EnergyTimeStampsAreCorrectlyAllocatedInReservedMemoryPage0)
{
	ExtlibEnergy energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	ReservedMemoryManager::DomainMemoryPool *domain_memory_map[EXTLIB_NUM_DOMAINS];
	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
		{
			domain_memory_map[domain_idx] = ReservedMemoryManager::get_domain_map(domain_idx, 0);
		}
	}

	int n_ts = RESERVED_ELEMENT_SIZE * 2;
	for (int i = 0; i < n_ts; i++)
	{
		auto *ts = extlib_read_energy_measurements(&energy_domains);

		size_t expected_ts_container_size = i + 2;
		EXPECT_EQ(ReservedMemoryManager::metadata.size(), 1)
			<< "Size of metadata container should remain 1" << " after energy timestamp has been created at index " << (i + 1) << " of reserved memory page 0.";
		ASSERT_EQ(ReservedMemoryManager::metadata[0].size(), expected_ts_container_size)
			<< "Size of container for energy timestamp metadata for reserved memory page 0 should be " << expected_ts_container_size
			<< " after energy timestamp has been created at index " << (i + 1) << " of the page" << " (" << ReservedMemoryManager::metadata[0].size()
			<< " != " << expected_ts_container_size << ").";
		ExtlibEnergyTimeStamp *expected_ts_ptr = &ReservedMemoryManager::metadata[0][i + 1];

		ASSERT_EQ(ts, expected_ts_ptr) << "Pointer to energy timestamp created at index " << (i + 1)
									   << " of reserved memory page 0 does not point to expected memory location.";

		size_t expected_reserved_memory_index = i + 1;
		ASSERT_EQ(ts->reserved_memory_page, 0) << "reserved_memory_page variable of energy timestamp created at index " << (i + 1)
											   << " of reserved memory page 0 should be 0.";
		ASSERT_EQ(ts->reserved_memory_index, expected_reserved_memory_index)
			<< "reserved_memory_index variable of energy timestamp created at index " << (i + 1) << " of reserved memory page 0 should be "
			<< expected_reserved_memory_index << " (" << ts->reserved_memory_index << " != " << expected_reserved_memory_index << ").";

		for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
		{
			unsigned int domain = 1 << domain_idx;
			if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
			{
				std::string domain_name = getEnergyDomainName(domain);
				EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].size(), 1)
					<< "Size of container for reserved memory pages of " << domain_name << " energy domain should remain 1"
					<< " after energy timestamp has been created at index " << (i + 1) << " of reserved memory page 0.";

				if (i == 0)
				{
					size_t expected_arr_size = domain_memory_map[domain_idx]->arr_size;
					ASSERT_EQ(ts->domain_data[domain_idx].arr_size, expected_arr_size)
						<< "arr_size variable of " << domain_name << " energy domain of energy timestamp created at index " << (i + 1)
						<< " of reserved memory page 0 should be " << expected_arr_size << " (" << ts->domain_data[domain_idx].arr_size
						<< " != " << expected_arr_size << ").";
				}

				int offset = ts->reserved_memory_index * ts->domain_data[domain_idx].arr_size;
				uint64_t *expected_counter_value_ptr = &domain_memory_map[domain_idx]->counter_value[offset];
				double *expected_energy_per_counter_ptr = &domain_memory_map[domain_idx]->energy_per_counter[offset];

				EXPECT_EQ(ts->domain_data[domain_idx].counter_value, expected_counter_value_ptr)
					<< "counter_value pointer of " << domain_name << " energy domain of energy timestamp created at index " << (i + 1)
					<< " of reserved memory page 0 does not point to expected memory location.";
				EXPECT_EQ(ts->domain_data[domain_idx].energy_per_counter, expected_energy_per_counter_ptr)
					<< "energy_per_counter pointer of " << domain_name << " energy domain of energy timestamp created at index " << (i + 1)
					<< " of reserved memory page 0 does not point to expected memory location.";
			}
		}

		uint64_t expected_entry_length = i + 2;
		EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 1)
			<< "Size of entry_length container should remain 1" << " after energy timestamp has been created at index " << (i + 1)
			<< " of reserved memory page 0.";
		EXPECT_EQ(ReservedMemoryManager::entry_length[0], expected_entry_length)
			<< "Number of entries in reserved memory page 0 should be " << expected_entry_length << " after energy timestamp has been created at index "
			<< (i + 1) << " of the page" << " (" << ReservedMemoryManager::entry_length[0] << " != " << expected_entry_length << ").";
		EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.empty())
			<< "empty_page_ids container should remain empty" << " after energy timestamp has been created at index " << (i + 1)
			<< " of reserved memory page 0.";
	}

	extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, NewReservedMemoryPageIsCreatedWhenPage0IsFull)
{
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

	int n_ts = (RESERVED_ELEMENT_SIZE * 2) + 1;
	for (int i = 0; i < n_ts; i++)
	{
		auto *ts = extlib_read_energy_measurements(&energy_domains);
	}

	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
		{
			ReservedMemoryManager::DomainMemoryPool *domain_memory_map = ReservedMemoryManager::get_domain_map(domain_idx, 1);

			std::string domain_name = getEnergyDomainName(domain);
			if (!domain_memory_map)
			{
				FAIL() << domain_name << " energy domain exists but pointer to its reserved memory page 1 is NULL/nullptr.";
				continue;
			}
			EXPECT_NE(domain_memory_map->counter_name, nullptr)
				<< domain_name << " energy domain exists but counter_name pointer of its reserved memory page 1 is NULL/nullptr.";
			EXPECT_NE(domain_memory_map->counter_value, nullptr)
				<< domain_name << " energy domain exists but counter_value pointer of its reserved memory page 1 is NULL/nullptr.";
			EXPECT_NE(domain_memory_map->energy_per_counter, nullptr)
				<< domain_name << " energy domain exists but energy_per_counter pointer of its reserved memory page 1 is NULL/nullptr.";

			size_t expected_arr_size = domain_arr_size[domain_idx];
			size_t expected_sizeof_counter_name = sizeof(char *) * domain_arr_size[domain_idx];
			size_t expected_sizeof_counter_value = sizeof(uint64_t) * domain_arr_size[domain_idx] * ReservedMemoryManager::reserved_memory_size;
			size_t expected_sizeof_energy_per_counter = sizeof(double) * domain_arr_size[domain_idx] * ReservedMemoryManager::reserved_memory_size;

			EXPECT_EQ(domain_memory_map->arr_size, expected_arr_size)
				<< "arr_size variable of reserved memory page 1 of existing " << domain_name << " energy domain should be " << expected_arr_size << " ("
				<< domain_memory_map->arr_size << " != " << expected_arr_size << ").";
			EXPECT_EQ(domain_memory_map->sizeof_counter_name, expected_sizeof_counter_name)
				<< "sizeof_counter_name variable of reserved memory page 1 of existing " << domain_name << " energy domain should be "
				<< expected_sizeof_counter_name << " (" << domain_memory_map->sizeof_counter_name << " != " << expected_sizeof_counter_name << ").";
			EXPECT_EQ(domain_memory_map->sizeof_counter_value, expected_sizeof_counter_value)
				<< "sizeof_counter_value variable of reserved memory page 1 of existing " << domain_name << " energy domain should be "
				<< expected_sizeof_counter_value << " (" << domain_memory_map->sizeof_counter_value << " != " << expected_sizeof_counter_value << ").";
			EXPECT_EQ(domain_memory_map->sizeof_energy_per_counter, expected_sizeof_energy_per_counter)
				<< "sizeof_energy_per_counter variable of reserved memory page 1 of existing " << domain_name << " energy domain should be "
				<< expected_sizeof_energy_per_counter << " (" << domain_memory_map->sizeof_energy_per_counter << " != " << expected_sizeof_energy_per_counter
				<< ").";
		}
	}

	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 2) << "Size of metadata container should be 2.";
	ASSERT_EQ(ReservedMemoryManager::metadata.count(1), 1) << "Container for energy timestamp metadata for reserved memory page 1 is missing.";
	EXPECT_EQ(ReservedMemoryManager::metadata[1].capacity(), ReservedMemoryManager::reserved_memory_size)
		<< "Capacity of container for energy timestamp metadata for reserved memory page 1 should be " << ReservedMemoryManager::reserved_memory_size << " ("
		<< ReservedMemoryManager::metadata[1].capacity() << " != " << ReservedMemoryManager::reserved_memory_size << ").";

	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 2) << "Size of entry_length container should be 2.";
	ASSERT_EQ(ReservedMemoryManager::entry_length.count(1), 1) << "Number of entries in reserved memory page 1 is missing.";
	EXPECT_EQ(ReservedMemoryManager::entry_length[1], 1) << "Number of entries in reserved memory page 1 should be 1.";

	extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, EnergyTimeStampsAreCorrectlyAllocatedInReservedMemoryPage1)
{
	ExtlibEnergy energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	int n_ts = RESERVED_ELEMENT_SIZE * 2;
	for (int i = 0; i < n_ts; i++)
	{
		auto *ts = extlib_read_energy_measurements(&energy_domains);
	}

	ReservedMemoryManager::DomainMemoryPool *domain_memory_map[EXTLIB_NUM_DOMAINS];
	for (int i = 0; i < n_ts; i++)
	{
		auto *ts = extlib_read_energy_measurements(&energy_domains);

		if (i == 0)
		{
			for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
			{
				unsigned int domain = 1 << domain_idx;
				if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
				{
					domain_memory_map[domain_idx] = ReservedMemoryManager::get_domain_map(domain_idx, 1);
				}
			}
		}

		size_t expected_ts_container_size = i + 1;
		EXPECT_EQ(ReservedMemoryManager::metadata.size(), 2)
			<< "Size of metadata container should remain 2" << " after energy timestamp has been created at index " << i << " of reserved memory page 1.";
		ASSERT_EQ(ReservedMemoryManager::metadata[1].size(), expected_ts_container_size)
			<< "Size of container for energy timestamp metadata for reserved memory page 1 should be " << expected_ts_container_size
			<< " after energy timestamp has been created at index " << i << " of the page" << " (" << ReservedMemoryManager::metadata[1].size()
			<< " != " << expected_ts_container_size << ").";
		ExtlibEnergyTimeStamp *expected_ts_ptr = &ReservedMemoryManager::metadata[1][i];

		ASSERT_EQ(ts, expected_ts_ptr) << "Pointer to energy timestamp created at index " << i
									   << " of reserved memory page 1 does not point to expected memory location.";

		size_t expected_reserved_memory_index = i;
		ASSERT_EQ(ts->reserved_memory_page, 1) << "reserved_memory_page variable of energy timestamp created at index " << i
											   << " of reserved memory page 1 should be 1.";
		ASSERT_EQ(ts->reserved_memory_index, expected_reserved_memory_index)
			<< "reserved_memory_index variable of energy timestamp created at index " << i << " of reserved memory page 1 should be "
			<< expected_reserved_memory_index << " (" << ts->reserved_memory_index << " != " << expected_reserved_memory_index << ").";

		for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
		{
			unsigned int domain = 1 << domain_idx;
			if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
			{
				std::string domain_name = getEnergyDomainName(domain);
				EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].size(), 2)
					<< "Size of container for reserved memory pages of " << domain_name << " energy domain should remain 2"
					<< " after energy timestamp has been created at index " << i << " of reserved memory page 1.";

				if (i == 0)
				{
					size_t expected_arr_size = domain_memory_map[domain_idx]->arr_size;
					ASSERT_EQ(ts->domain_data[domain_idx].arr_size, expected_arr_size)
						<< "arr_size variable of " << domain_name << " energy domain of energy timestamp created at index " << i
						<< " of reserved memory page 1 should be " << expected_arr_size << " (" << ts->domain_data[domain_idx].arr_size
						<< " != " << expected_arr_size << ").";
				}

				int offset = ts->reserved_memory_index * ts->domain_data[domain_idx].arr_size;
				uint64_t *expected_counter_value_ptr = &domain_memory_map[domain_idx]->counter_value[offset];
				double *expected_energy_per_counter_ptr = &domain_memory_map[domain_idx]->energy_per_counter[offset];

				EXPECT_EQ(ts->domain_data[domain_idx].counter_value, expected_counter_value_ptr)
					<< "counter_value pointer of " << domain_name << " energy domain of energy timestamp created at index " << i
					<< " of reserved memory page 1 does not point to expected memory location.";
				EXPECT_EQ(ts->domain_data[domain_idx].energy_per_counter, expected_energy_per_counter_ptr)
					<< "energy_per_counter pointer of " << domain_name << " energy domain of energy timestamp created at index " << i
					<< " of reserved memory page 1 does not point to expected memory location.";
			}
		}

		uint64_t expected_entry_length = i + 1;
		EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 2)
			<< "Size of entry_length container should remain 2" << " after energy timestamp has been created at index " << i << " of reserved memory page 1.";
		EXPECT_EQ(ReservedMemoryManager::entry_length[1], expected_entry_length)
			<< "Number of entries in reserved memory page 1 should be " << expected_entry_length << " after energy timestamp has been created at index " << i
			<< " of the page" << " (" << ReservedMemoryManager::entry_length[1] << " != " << expected_entry_length << ").";
		EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.empty())
			<< "empty_page_ids container should remain empty" << " after energy timestamp has been created at index " << i << " of reserved memory page 1.";
	}

	extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, ResultEnergyTimeStampsAreAlwaysAllocatedAtIndex0OfReservedMemoryPage0)
{
	ExtlibEnergy energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	ExtlibEnergyTimeStamp *expected_result_ts_ptr = &ReservedMemoryManager::metadata[0][0];

	auto *ts1 = extlib_read_energy_measurements(&energy_domains);
	do_work(2);
	auto *ts2 = extlib_read_energy_measurements(&energy_domains);

	int n_ts = (RESERVED_ELEMENT_SIZE * 2) - 2;
	for (int i = 0; i < n_ts; i++)
	{
		auto *ts = extlib_read_energy_measurements(&energy_domains);
	}

	auto *result1 = extlib_calc_energy_consumption(ts1, ts2);

	ASSERT_EQ(result1, expected_result_ts_ptr) << "Pointer to result energy timestamp does not point to expected memory location.";
	EXPECT_EQ(result1->reserved_memory_page, 0) << "reserved_memory_page variable should be 0.";
	EXPECT_EQ(result1->reserved_memory_index, 0) << "reserved_memory_index variable should be 0.";

	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
		{
			std::string domain_name = getEnergyDomainName(domain);
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].size(), 1)
				<< "Size of container for reserved memory pages of " << domain_name << " energy domain should be 1.";

			ReservedMemoryManager::DomainMemoryPool *domain_memory_map = ReservedMemoryManager::get_domain_map(domain_idx, 0);

			size_t expected_arr_size = domain_memory_map->arr_size;
			EXPECT_EQ(result1->domain_data[domain_idx].arr_size, expected_arr_size)
				<< "arr_size variable of " << domain_name << " energy domain should be " << expected_arr_size << " ("
				<< result1->domain_data[domain_idx].arr_size << " != " << expected_arr_size << ").";

			uint64_t *expected_counter_value_ptr = &domain_memory_map->counter_value[0];
			double *expected_energy_per_counter_ptr = &domain_memory_map->energy_per_counter[0];

			EXPECT_EQ(result1->domain_data[domain_idx].counter_value, expected_counter_value_ptr)
				<< "counter_value pointer of " << domain_name << " energy domain does not point to expected memory location.";
			EXPECT_EQ(result1->domain_data[domain_idx].energy_per_counter, expected_energy_per_counter_ptr)
				<< "energy_per_counter pointer of " << domain_name << " energy domain does not point to expected memory location.";
		}
	}

	size_t expected_ts_container_size = (RESERVED_ELEMENT_SIZE * 2) + 1;
	uint64_t expected_entry_length = (RESERVED_ELEMENT_SIZE * 2) + 1;

	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 1) << "Size of metadata container should be 1.";
	EXPECT_EQ(ReservedMemoryManager::metadata[0].size(), expected_ts_container_size)
		<< "Size of container for energy timestamp metadata for reserved memory page 0 should be " << expected_ts_container_size << " ("
		<< ReservedMemoryManager::metadata[0].size() << " != " << expected_ts_container_size << ").";
	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 1) << "Size of entry_length container should be 1.";
	EXPECT_EQ(ReservedMemoryManager::entry_length[0], expected_entry_length)
		<< "Number of entries in reserved memory page 0 should be " << expected_entry_length << " (" << ReservedMemoryManager::entry_length[0]
		<< " != " << expected_entry_length << ").";
	EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.empty()) << "empty_page_ids container should be empty.";

	auto *ts3 = extlib_read_energy_measurements(&energy_domains);
	do_work(2);
	auto *ts4 = extlib_read_energy_measurements(&energy_domains);

	for (int i = 0; i < n_ts; i++)
	{
		auto *ts = extlib_read_energy_measurements(&energy_domains);
	}

	auto *result2 = extlib_calc_energy_consumption(ts3, ts4);

	ASSERT_EQ(result2, expected_result_ts_ptr) << "Pointer to result energy timestamp does not point to expected memory location.";
	EXPECT_EQ(result2->reserved_memory_page, 0) << "reserved_memory_page variable should be 0.";
	EXPECT_EQ(result2->reserved_memory_index, 0) << "reserved_memory_index variable should be 0.";

	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
		{
			std::string domain_name = getEnergyDomainName(domain);
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].size(), 2)
				<< "Size of container for reserved memory pages of " << domain_name << " energy domain should be 2.";

			ReservedMemoryManager::DomainMemoryPool *domain_memory_map = ReservedMemoryManager::get_domain_map(domain_idx, 0);

			size_t expected_arr_size = domain_memory_map->arr_size;
			EXPECT_EQ(result2->domain_data[domain_idx].arr_size, expected_arr_size)
				<< "arr_size variable of " << domain_name << " energy domain should be " << expected_arr_size << " ("
				<< result2->domain_data[domain_idx].arr_size << " != " << expected_arr_size << ").";

			uint64_t *expected_counter_value_ptr = &domain_memory_map->counter_value[0];
			double *expected_energy_per_counter_ptr = &domain_memory_map->energy_per_counter[0];

			EXPECT_EQ(result2->domain_data[domain_idx].counter_value, expected_counter_value_ptr)
				<< "counter_value pointer of " << domain_name << " energy domain does not point to expected memory location.";
			EXPECT_EQ(result2->domain_data[domain_idx].energy_per_counter, expected_energy_per_counter_ptr)
				<< "energy_per_counter pointer of " << domain_name << " energy domain does not point to expected memory location.";
		}
	}

	expected_ts_container_size = RESERVED_ELEMENT_SIZE * 2;
	expected_entry_length = RESERVED_ELEMENT_SIZE * 2;

	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 2) << "Size of metadata container should be 2.";
	EXPECT_EQ(ReservedMemoryManager::metadata[1].size(), expected_ts_container_size)
		<< "Size of container for energy timestamp metadata for reserved memory page 1 should be " << expected_ts_container_size << " ("
		<< ReservedMemoryManager::metadata[1].size() << " != " << expected_ts_container_size << ").";
	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 2) << "Size of entry_length container should be 2.";
	EXPECT_EQ(ReservedMemoryManager::entry_length[1], expected_entry_length)
		<< "Number of entries in reserved memory page 1 should be " << expected_entry_length << " (" << ReservedMemoryManager::entry_length[1]
		<< " != " << expected_entry_length << ").";
	EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.empty()) << "empty_page_ids container should be empty.";

	extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, EnergyDomainCounterNamesAreSharedAcrossReservedMemoryPages)
{
	ExtlibEnergy energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	ExtlibEnergyTimeStamp *ts_array[2][RESERVED_ELEMENT_SIZE * 2];
	int n_ts = RESERVED_ELEMENT_SIZE * 2;
	for (int i = 0; i < n_ts; i++)
	{
		ts_array[0][i] = extlib_read_energy_measurements(&energy_domains);
	}
	do_work(2);
	for (int i = 0; i < n_ts; i++)
	{
		ts_array[1][i] = extlib_read_energy_measurements(&energy_domains);
	}
	auto *result = extlib_calc_energy_consumption(ts_array[0][0], ts_array[1][0]);

	ReservedMemoryManager::DomainMemoryPool *domain_memory_map[2][EXTLIB_NUM_DOMAINS];
	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
		{
			domain_memory_map[0][domain_idx] = ReservedMemoryManager::get_domain_map(domain_idx, 0);
			domain_memory_map[1][domain_idx] = ReservedMemoryManager::get_domain_map(domain_idx, 1);

			std::string domain_name = getEnergyDomainName(domain);
			ASSERT_EQ(domain_memory_map[0][domain_idx]->counter_name, domain_memory_map[1][domain_idx]->counter_name)
				<< "counter_name pointers of reserved memory page 0 and 1 of " << domain_name << " energy domain do not point to the same memory location.";
		}
	}

	int n_pages = 2;
	for (int i = 0; i < n_pages; i++)
	{
		for (int j = 0; j < n_ts; j++)
		{
			for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
			{
				unsigned int domain = 1 << domain_idx;
				if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
				{
					std::string domain_name = getEnergyDomainName(domain);
					char **expected_counter_name_ptr = domain_memory_map[i][domain_idx]->counter_name;
					EXPECT_EQ(ts_array[i][j]->domain_data[domain_idx].counter_name, expected_counter_name_ptr)
						<< "counter_name pointer of " << domain_name << " energy domain of energy timestamp created at index " << ((i == 0) ? (j + 1) : j)
						<< " of reserved memory page " << i << " does not point to expected memory location.";
				}
			}
		}
	}
	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
		{
			std::string domain_name = getEnergyDomainName(domain);
			char **expected_counter_name_ptr = domain_memory_map[0][domain_idx]->counter_name;
			EXPECT_EQ(result->domain_data[domain_idx].counter_name, expected_counter_name_ptr)
				<< "counter_name pointer of " << domain_name << " energy domain of result energy timestamp does not point to expected memory location.";
		}
	}

	extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, FreeEnergyTimestampCorrectlyDeallocatesEnergyTimeStamps)
{
	ExtlibEnergy energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	ExtlibEnergyTimeStamp *ts_array[2][RESERVED_ELEMENT_SIZE * 2];
	int n_ts = RESERVED_ELEMENT_SIZE * 2;
	for (int i = 0; i < n_ts; i++)
	{
		ts_array[0][i] = extlib_read_energy_measurements(&energy_domains);
	}
	do_work(2);
	for (int i = 0; i < n_ts; i++)
	{
		ts_array[1][i] = extlib_read_energy_measurements(&energy_domains);
	}
	auto *result = extlib_calc_energy_consumption(ts_array[0][0], ts_array[1][0]);

	int n_pages = 2;
	for (int i = (n_pages - 1); i >= 0; i--)
	{
		ReservedMemoryManager::DomainMemoryPool *domain_memory_map[EXTLIB_NUM_DOMAINS];
		for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
		{
			unsigned int domain = 1 << domain_idx;
			if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
			{
				domain_memory_map[domain_idx] = ReservedMemoryManager::get_domain_map(domain_idx, i);
			}
		}

		for (int j = 0; j < n_ts; j++)
		{
			ExtlibEnergyTimeStamp *ts = ts_array[i][j];

			extlib_free_energy_timestamp(ts_array[i][j]);

			size_t expected_ts_container_size = (i == 0) ? ((RESERVED_ELEMENT_SIZE * 2) + 1) : (RESERVED_ELEMENT_SIZE * 2);
			ASSERT_EQ(ReservedMemoryManager::metadata[i].size(), expected_ts_container_size)
				<< "Size of container for energy timestamp metadata for reserved memory page " << i << " should remain " << expected_ts_container_size
				<< " after energy timestamp at index " << ((i == 0) ? (j + 1) : j) << " of the page has been freed" << " ("
				<< ReservedMemoryManager::metadata[i].size() << " != " << expected_ts_container_size << ").";

			EXPECT_EQ(ts->id, 0.0) << "ID of energy timestamp at index " << ((i == 0) ? (j + 1) : j) << " of reserved memory page " << i
								   << " should be 0 after the timestamp has been freed.";
			for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
			{
				unsigned int domain = 1 << domain_idx;
				std::string domain_name = getEnergyDomainName(domain);
				EXPECT_EQ(ts->domain_data[domain_idx].domain_id, 0)
					<< "domain_id variable of " << domain_name << " energy domain of energy timestamp at index " << ((i == 0) ? (j + 1) : j)
					<< " of reserved memory page " << i << " should be 0 after the timestamp has been freed.";
				EXPECT_EQ(ts->domain_data[domain_idx].arr_size, 0)
					<< "arr_size variable of " << domain_name << " energy domain of energy timestamp at index " << ((i == 0) ? (j + 1) : j)
					<< " of reserved memory page " << i << " should be 0 after the timestamp has been freed.";
				EXPECT_EQ(ts->domain_data[domain_idx].counter_name, nullptr)
					<< "counter_name pointer of " << domain_name << " energy domain of energy timestamp at index " << ((i == 0) ? (j + 1) : j)
					<< " of reserved memory page " << i << " should be NULL/nullptr after the timestamp has been freed.";
				EXPECT_EQ(ts->domain_data[domain_idx].counter_value, nullptr)
					<< "counter_value pointer of " << domain_name << " energy domain of energy timestamp at index " << ((i == 0) ? (j + 1) : j)
					<< " of reserved memory page " << i << " should be NULL/nullptr after the timestamp has been freed.";
				EXPECT_EQ(ts->domain_data[domain_idx].energy_per_counter, nullptr)
					<< "energy_per_counter pointer of " << domain_name << " energy domain of energy timestamp at index " << ((i == 0) ? (j + 1) : j)
					<< " of reserved memory page " << i << " should be NULL/nullptr after the timestamp has been freed.";
				EXPECT_EQ(ts->domain_data[domain_idx].energy_total, 0.0)
					<< "energy_total variable of " << domain_name << " energy domain of energy timestamp at index " << ((i == 0) ? (j + 1) : j)
					<< " of reserved memory page " << i << " should be 0 after the timestamp has been freed.";
			}
			EXPECT_EQ(ts->energy_total, 0.0) << "energy_total variable of energy timestamp at index " << ((i == 0) ? (j + 1) : j) << " of reserved memory page "
											 << i << " should be 0 after the timestamp has been freed.";
			EXPECT_EQ(ts->reserved_memory_page, -1) << "reserved_memory_page variable of energy timestamp at index " << ((i == 0) ? (j + 1) : j)
													<< " of reserved memory page " << i << " should be -1 after the timestamp has been freed.";
			EXPECT_EQ(ts->reserved_memory_index, -1) << "reserved_memory_index variable of energy timestamp at index " << ((i == 0) ? (j + 1) : j)
													 << " of reserved memory page " << i << " should be -1 after the timestamp has been freed.";

			for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
			{
				unsigned int domain = 1 << domain_idx;
				if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
				{
					int offset = ((i == 0) ? (j + 1) : j) * domain_memory_map[domain_idx]->arr_size;
					uint64_t *counter_value = &domain_memory_map[domain_idx]->counter_value[offset];
					double *energy_per_counter = &domain_memory_map[domain_idx]->energy_per_counter[offset];

					std::string domain_name = getEnergyDomainName(domain);
					for (int k = 0; k < domain_memory_map[domain_idx]->arr_size; k++)
					{
						EXPECT_EQ(counter_value[k], 0) << "Value at index " << k << " of array to which counter_value pointer of " << domain_name
													   << " energy domain of energy timestamp at index " << ((i == 0) ? (j + 1) : j)
													   << " of reserved memory page " << i << " pointed to should be 0 after the timestamp has been freed.";
						EXPECT_EQ(energy_per_counter[k], 0.0)
							<< "Value at index " << k << " of array to which energy_per_counter pointer of " << domain_name
							<< " energy domain of energy timestamp at index " << ((i == 0) ? (j + 1) : j) << " of reserved memory page " << i
							<< " pointed to should be 0 after the timestamp has been freed.";
					}
				}
			}

			uint64_t expected_entry_length = ((i == 0) ? ((RESERVED_ELEMENT_SIZE * 2) + 1) : (RESERVED_ELEMENT_SIZE * 2)) - (j + 1);
			EXPECT_EQ(ReservedMemoryManager::entry_length[i], expected_entry_length)
				<< "Number of entries in reserved memory page " << i << " should be " << expected_entry_length << " after energy timestamp at index "
				<< ((i == 0) ? (j + 1) : j) << " of the page has been freed" << " (" << ReservedMemoryManager::entry_length[i]
				<< " != " << expected_entry_length << ").";
		}
	}

	ExtlibEnergyTimeStamp *result_ts = result;

	extlib_free_energy_timestamp(result);

	size_t expected_ts_container_size = (RESERVED_ELEMENT_SIZE * 2) + 1;
	ASSERT_EQ(ReservedMemoryManager::metadata[0].size(), expected_ts_container_size)
		<< "Size of container for energy timestamp metadata for reserved memory page 0 should remain " << expected_ts_container_size
		<< " after result energy timestamp has been freed" << " (" << ReservedMemoryManager::metadata[0].size() << " != " << expected_ts_container_size << ").";

	EXPECT_EQ(result_ts->reserved_memory_page, 0) << "reserved_memory_page variable of result energy timestamp should be 0"
												  << " after the timestamp has been freed.";
	EXPECT_EQ(result_ts->reserved_memory_index, 0) << "reserved_memory_index variable of result energy timestamp should be 0"
												   << " after the timestamp has been freed.";

	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
		{
			ReservedMemoryManager::DomainMemoryPool *domain_memory_map = ReservedMemoryManager::get_domain_map(domain_idx, 0);

			uint64_t *counter_value = &domain_memory_map->counter_value[0];
			double *energy_per_counter = &domain_memory_map->energy_per_counter[0];

			std::string domain_name = getEnergyDomainName(domain);
			for (int k = 0; k < domain_memory_map->arr_size; k++)
			{
				EXPECT_EQ(counter_value[k], 0) << "Value at index " << k << " of array to which counter_value pointer of " << domain_name
											   << " energy domain of result energy timestamp pointed to should be 0" << " after the timestamp has been freed.";
				EXPECT_EQ(energy_per_counter[k], 0.0) << "Value at index " << k << " of array to which energy_per_counter pointer of " << domain_name
													  << " energy domain of result energy timestamp pointed to should be 0"
													  << " after the timestamp has been freed.";
			}
		}
	}

	EXPECT_EQ(ReservedMemoryManager::entry_length[0], 1)
		<< "Number of entries in reserved memory page 0 should remain 1" << " after result energy timestamp has been freed.";

	extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, EnergyTimeStampsAreAllocatedInEmptyReservedMemorySlotsFirst)
{
	ExtlibEnergy energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	ExtlibEnergyTimeStamp *ts_array[2][RESERVED_ELEMENT_SIZE * 2];
	int n_pages = 2;
	int n_ts = RESERVED_ELEMENT_SIZE * 2;
	for (int i = 0; i < n_pages; i++)
	{
		for (int j = 0; j < n_ts; j++)
		{
			ts_array[i][j] = extlib_read_energy_measurements(&energy_domains);
		}
	}
	ExtlibEnergyTimeStamp *ts = extlib_read_energy_measurements(&energy_domains);

	for (int i = 0; i < n_pages; i++)
	{
		for (int j = 1; j < n_ts; j += 2)
		{
			extlib_free_energy_timestamp(ts_array[i][j]);
		}

		ReservedMemoryManager::DomainMemoryPool *domain_memory_map[EXTLIB_NUM_DOMAINS];
		for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
		{
			unsigned int domain = 1 << domain_idx;
			if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
			{
				domain_memory_map[domain_idx] = ReservedMemoryManager::get_domain_map(domain_idx, i);
			}
		}

		for (int j = 1; j < n_ts; j += 2)
		{
			auto *ts = extlib_read_energy_measurements(&energy_domains);

			size_t expected_ts_container_size = (i == 0) ? ((RESERVED_ELEMENT_SIZE * 2) + 1) : (RESERVED_ELEMENT_SIZE * 2);
			EXPECT_EQ(ReservedMemoryManager::metadata.size(), 3)
				<< "Size of metadata container should remain 3" << " after energy timestamp has been created at index " << ((i == 0) ? (j + 1) : j)
				<< " of reserved memory page " << i << ".";
			ASSERT_EQ(ReservedMemoryManager::metadata[i].size(), expected_ts_container_size)
				<< "Size of container for energy timestamp metadata for reserved memory page " << i << " should remain " << expected_ts_container_size
				<< " after energy timestamp has been created at index " << ((i == 0) ? (j + 1) : j) << " of the page" << " ("
				<< ReservedMemoryManager::metadata[i].size() << " != " << expected_ts_container_size << ").";
			ExtlibEnergyTimeStamp *expected_ts_ptr = &ReservedMemoryManager::metadata[i][(i == 0) ? (j + 1) : j];

			ASSERT_EQ(ts, expected_ts_ptr) << "Pointer to energy timestamp created at index " << ((i == 0) ? (j + 1) : j) << " of reserved memory page " << i
										   << " does not point to expected memory location.";

			size_t expected_reserved_memory_page = i;
			size_t expected_reserved_memory_index = (i == 0) ? (j + 1) : j;

			ASSERT_EQ(ts->reserved_memory_page, expected_reserved_memory_page)
				<< "reserved_memory_page variable of energy timestamp created at index " << ((i == 0) ? (j + 1) : j) << " of reserved memory page " << i
				<< " should be " << expected_reserved_memory_page << " (" << ts->reserved_memory_page << " != " << expected_reserved_memory_page << ").";
			ASSERT_EQ(ts->reserved_memory_index, expected_reserved_memory_index)
				<< "reserved_memory_index variable of energy timestamp created at index " << ((i == 0) ? (j + 1) : j) << " of reserved memory page " << i
				<< " should be " << expected_reserved_memory_index << " (" << ts->reserved_memory_index << " != " << expected_reserved_memory_index << ").";

			for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
			{
				unsigned int domain = 1 << domain_idx;
				if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
				{
					std::string domain_name = getEnergyDomainName(domain);
					EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].size(), 3)
						<< "Size of container for reserved memory pages of " << domain_name << " energy domain should remain 3"
						<< " after energy timestamp has been created at index " << ((i == 0) ? (j + 1) : j) << " of reserved memory page " << i << ".";

					int offset = ts->reserved_memory_index * ts->domain_data[domain_idx].arr_size;
					uint64_t *expected_counter_value_ptr = &domain_memory_map[domain_idx]->counter_value[offset];
					double *expected_energy_per_counter_ptr = &domain_memory_map[domain_idx]->energy_per_counter[offset];

					EXPECT_EQ(ts->domain_data[domain_idx].counter_value, expected_counter_value_ptr)
						<< "counter_value pointer of " << domain_name << " energy domain of energy timestamp created at index " << ((i == 0) ? (j + 1) : j)
						<< " of reserved memory page " << i << " does not point to expected memory location.";
					EXPECT_EQ(ts->domain_data[domain_idx].energy_per_counter, expected_energy_per_counter_ptr)
						<< "energy_per_counter pointer of " << domain_name << " energy domain of energy timestamp created at index " << ((i == 0) ? (j + 1) : j)
						<< " of reserved memory page " << i << " does not point to expected memory location.";
				}
			}

			uint64_t expected_entry_length = ((i == 0) ? (RESERVED_ELEMENT_SIZE + 1) : RESERVED_ELEMENT_SIZE) + ((j + 1) / 2);
			EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 3)
				<< "Size of entry_length container should remain 3" << " after energy timestamp has been created at index " << ((i == 0) ? (j + 1) : j)
				<< " of reserved memory page " << i << ".";
			EXPECT_EQ(ReservedMemoryManager::entry_length[i], expected_entry_length)
				<< "Number of entries in reserved memory page " << i << " should be " << expected_entry_length
				<< " after energy timestamp has been created at index " << ((i == 0) ? (j + 1) : j) << " of the page" << " ("
				<< ReservedMemoryManager::entry_length[i] << " != " << expected_entry_length << ").";
			EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.empty())
				<< "empty_page_ids container should remain empty" << " after energy timestamp has been created at index " << ((i == 0) ? (j + 1) : j)
				<< " of reserved memory page " << i << ".";
		}
	}

	extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, ReservedMemoryPage3IsCreatedWhenPages0And2AreFullAndPage1HasBeenRemoved)
{
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

	ExtlibEnergyTimeStamp *ts_array[3][RESERVED_ELEMENT_SIZE * 2];
	int n_pages = 3;
	int n_ts = RESERVED_ELEMENT_SIZE * 2;
	for (int i = 0; i < n_pages; i++)
	{
		for (int j = 0; j < n_ts; j++)
		{
			ts_array[i][j] = extlib_read_energy_measurements(&energy_domains);
		}
	}

	for (int i = 0; i < n_ts; i++)
	{
		extlib_free_energy_timestamp(ts_array[1][i]);
	}
	extlib_shrink_to_fit();
	auto *ts = extlib_read_energy_measurements(&energy_domains);

	EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.empty()) << "empty_page_ids container should be empty.";

	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain)))
		{
			ReservedMemoryManager::DomainMemoryPool *domain_memory_map = ReservedMemoryManager::get_domain_map(domain_idx, 3);

			std::string domain_name = getEnergyDomainName(domain);
			if (!domain_memory_map)
			{
				FAIL() << domain_name << " energy domain exists but pointer to its reserved memory page 3 is NULL/nullptr.";
				continue;
			}
			EXPECT_NE(domain_memory_map->counter_name, nullptr)
				<< domain_name << " energy domain exists but counter_name pointer of its reserved memory page 3 is NULL/nullptr.";
			EXPECT_NE(domain_memory_map->counter_value, nullptr)
				<< domain_name << " energy domain exists but counter_value pointer of its reserved memory page 3 is NULL/nullptr.";
			EXPECT_NE(domain_memory_map->energy_per_counter, nullptr)
				<< domain_name << " energy domain exists but energy_per_counter pointer of its reserved memory page 3 is NULL/nullptr.";

			size_t expected_arr_size = domain_arr_size[domain_idx];
			size_t expected_sizeof_counter_name = sizeof(char *) * domain_arr_size[domain_idx];
			size_t expected_sizeof_counter_value = sizeof(uint64_t) * domain_arr_size[domain_idx] * ReservedMemoryManager::reserved_memory_size;
			size_t expected_sizeof_energy_per_counter = sizeof(double) * domain_arr_size[domain_idx] * ReservedMemoryManager::reserved_memory_size;

			EXPECT_EQ(domain_memory_map->arr_size, expected_arr_size)
				<< "arr_size variable of reserved memory page 3 of existing " << domain_name << " energy domain should be " << expected_arr_size << " ("
				<< domain_memory_map->arr_size << " != " << expected_arr_size << ").";
			EXPECT_EQ(domain_memory_map->sizeof_counter_name, expected_sizeof_counter_name)
				<< "sizeof_counter_name variable of reserved memory page 3 of existing " << domain_name << " energy domain should be "
				<< expected_sizeof_counter_name << " (" << domain_memory_map->sizeof_counter_name << " != " << expected_sizeof_counter_name << ").";
			EXPECT_EQ(domain_memory_map->sizeof_counter_value, expected_sizeof_counter_value)
				<< "sizeof_counter_value variable of reserved memory page 3 of existing " << domain_name << " energy domain should be "
				<< expected_sizeof_counter_value << " (" << domain_memory_map->sizeof_counter_value << " != " << expected_sizeof_counter_value << ").";
			EXPECT_EQ(domain_memory_map->sizeof_energy_per_counter, expected_sizeof_energy_per_counter)
				<< "sizeof_energy_per_counter variable of reserved memory page 3 of existing " << domain_name << " energy domain should be "
				<< expected_sizeof_energy_per_counter << " (" << domain_memory_map->sizeof_energy_per_counter << " != " << expected_sizeof_energy_per_counter
				<< ").";
		}
	}

	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 3) << "Size of metadata container should be 3.";
	ASSERT_EQ(ReservedMemoryManager::metadata.count(3), 1) << "Container for energy timestamp metadata for reserved memory page 3 is missing.";
	EXPECT_EQ(ReservedMemoryManager::metadata[3].capacity(), ReservedMemoryManager::reserved_memory_size)
		<< "Capacity of container for energy timestamp metadata for reserved memory page 3 should be " << ReservedMemoryManager::reserved_memory_size << " ("
		<< ReservedMemoryManager::metadata[1].capacity() << " != " << ReservedMemoryManager::reserved_memory_size << ").";

	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 3) << "Size of entry_length container should be 3.";
	ASSERT_EQ(ReservedMemoryManager::entry_length.count(3), 1) << "Number of entries in reserved memory page 3 is missing.";
	EXPECT_EQ(ReservedMemoryManager::entry_length[3], 1) << "Number of entries in reserved memory page 3 should be 1.";

	extlib_close(&energy_domains);
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
	EXPECT_EQ(ReservedMemoryManager::domain_memory_pool.size(), 0) << "Size of domain_memory_pool container should be 0 after close.";
	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 0) << "Size of metadata container should be 0 after close.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 0) << "Size of entry_length container should be 0 after close.";
	EXPECT_EQ(ReservedMemoryManager::empty_page_ids.size(), 0) << "Size of empty_page_ids container should be 0 after close.";
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
	EXPECT_EQ(ReservedMemoryManager::domain_memory_pool.size(), 0) << "Size of domain_memory_pool container should be 0 after close.";
	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 0) << "Size of metadata container should be 0 after close.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 0) << "Size of entry_length container should be 0 after close.";
	EXPECT_EQ(ReservedMemoryManager::empty_page_ids.size(), 0) << "Size of empty_page_ids container should be 0 after close.";

	// make sure energy_domains is in reset state.
	EXPECT_EQ(energy_domains.value, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NO_SELECTION) << "All energy domains should be disabled after close.";
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

TEST(MemoryManagerTest, EmptyReservedMemoryPageOfNonZeroIndexIsRemovedWhenConditionIsMet)
{
    ExtlibEnergy energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	ExtlibEnergyTimeStamp *ts_array[2][RESERVED_ELEMENT_SIZE * 2];
	int n_pages = 2;
	int n_ts = RESERVED_ELEMENT_SIZE * 2;
	for (int i = 0; i < n_pages; i++)
	{
		for (int j = 0; j < n_ts; j++)
		{
			ts_array[i][j] = extlib_read_energy_measurements(&energy_domains);
		}
	}
	ExtlibEnergyTimeStamp *ts = extlib_read_energy_measurements(&energy_domains);

	n_ts = EXTLIB_EMPTY_SLOT_THRESHOLD;
	for (int i = 0; i < n_ts; i++)
	{
		extlib_free_energy_timestamp(ts_array[0][i]);
	}

	uint64_t expected_total_empty_slots_count = EXTLIB_EMPTY_SLOT_THRESHOLD;
	EXPECT_EQ(ReservedMemoryManager::count_total_empty_slots_before(1), expected_total_empty_slots_count)
		<< "Number of total empty slots in reserved memory page 0 should be " << expected_total_empty_slots_count << " ("
		<< ReservedMemoryManager::count_total_empty_slots_before(1) << " != " << expected_total_empty_slots_count << ").";

	n_ts = RESERVED_ELEMENT_SIZE * 2;
	for (int i = 0; i < n_ts; i++)
	{
		extlib_free_energy_timestamp(ts_array[1][i]);
	}

	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
		{
			std::string domain_name = getEnergyDomainName(domain);
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].size(), 2)
				<< "Size of container for reserved memory pages of " << domain_name << " energy domain should be 2"
				<< " after reserved memory page 1 has been removed.";
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].count(1), 0)
				<< "Reserved memory page 1 of " << domain_name << " energy domain should be removed.";
		}
	}
	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 2) << "Size of metadata container should be 2" << " after reserved memory page 1 has been removed.";
	EXPECT_EQ(ReservedMemoryManager::metadata.count(1), 0)
		<< "Container for energy timestamp metadata for reserved memory page 1 should be removed" << " after the page has been removed.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 2)
		<< "Size of entry_length container should be 2" << " after reserved memory page 1 has been removed.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.count(1), 0)
		<< "Number of entries in reserved memory page 1 should be removed" << " after the page has been removed.";
	EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.empty())
		<< "empty_page_ids container should be empty" << " after reserved memory page 1 has been removed.";

	extlib_close(&energy_domains);

	energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	n_ts = RESERVED_ELEMENT_SIZE * 2;
	for (int i = 0; i < n_pages; i++)
	{
		for (int j = 0; j < n_ts; j++)
		{
			ts_array[i][j] = extlib_read_energy_measurements(&energy_domains);
		}
	}
	ts = extlib_read_energy_measurements(&energy_domains);

	n_ts = EXTLIB_EMPTY_SLOT_THRESHOLD - 1;
	for (int i = 0; i < n_pages; i++)
	{
		for (int j = 0; j < n_ts; j++)
		{
			extlib_free_energy_timestamp(ts_array[i][j]);
		}
	}

	expected_total_empty_slots_count = (EXTLIB_EMPTY_SLOT_THRESHOLD - 1) * n_pages;
	EXPECT_EQ(ReservedMemoryManager::count_total_empty_slots_before(2), expected_total_empty_slots_count)
		<< "Number of total empty slots in reserved memory pages 0 and 1 should be " << expected_total_empty_slots_count << " ("
		<< ReservedMemoryManager::count_total_empty_slots_before(2) << " != " << expected_total_empty_slots_count << ").";

	extlib_free_energy_timestamp(ts);

	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
		{
			std::string domain_name = getEnergyDomainName(domain);
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].size(), 2)
				<< "Size of container for reserved memory pages of " << domain_name << " energy domain should be 2"
				<< " after reserved memory page 2 has been removed.";
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].count(2), 0)
				<< "Reserved memory page 2 of " << domain_name << " energy domain should be removed.";
		}
	}
	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 2) << "Size of metadata container should be 2" << " after reserved memory page 2 has been removed.";
	EXPECT_EQ(ReservedMemoryManager::metadata.count(2), 0)
		<< "Container for energy timestamp metadata for reserved memory page 2 should be removed" << " after the page has been removed.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 2)
		<< "Size of entry_length container should be 2" << " after reserved memory page 2 has been removed.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.count(2), 0)
		<< "Number of entries in reserved memory page 2 should be removed" << " after the page has been removed.";
	EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.empty())
		<< "empty_page_ids container should be empty" << " after reserved memory page 2 has been removed.";

	extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, EmptyReservedMemoryPageOfNonZeroIndexIsNotRemovedWhenConditionIsNotMet)
{
    ExtlibEnergy energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	ExtlibEnergyTimeStamp *ts_array[2][RESERVED_ELEMENT_SIZE * 2];
	int n_pages = 2;
	int n_ts = RESERVED_ELEMENT_SIZE * 2;
	for (int i = 0; i < n_pages; i++)
	{
		for (int j = 0; j < n_ts; j++)
		{
			ts_array[i][j] = extlib_read_energy_measurements(&energy_domains);
		}
	}
	ExtlibEnergyTimeStamp *ts = extlib_read_energy_measurements(&energy_domains);

	for (int i = 0; i < n_ts; i++)
	{
		extlib_free_energy_timestamp(ts_array[1][i]);
	}

	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
		{
			std::string domain_name = getEnergyDomainName(domain);
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].size(), 3)
				<< "Size of container for reserved memory pages of " << domain_name << " energy domain should be 3.";
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].count(1), 1)
				<< "Reserved memory page 1 of " << domain_name << " energy domain is missing.";
		}
	}
	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 3) << "Size of metadata container should be 3.";
	EXPECT_EQ(ReservedMemoryManager::metadata.count(1), 1) << "Container for energy timestamp metadata for reserved memory page 1 is missing.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 3) << "Size of entry_length container should be 3.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.count(1), 1) << "Number of entries in reserved memory page 1 is missing.";
	EXPECT_EQ(ReservedMemoryManager::empty_page_ids.size(), 1) << "Size of empty_page_ids container should be 1.";
	EXPECT_EQ(ReservedMemoryManager::empty_page_ids.count(1), 1) << "Reserved memory page 1 is empty but its ID is missing in empty_page_ids countainer.";

	extlib_close(&energy_domains);

	energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	for (int i = 0; i < n_pages; i++)
	{
		for (int j = 0; j < n_ts; j++)
		{
			ts_array[i][j] = extlib_read_energy_measurements(&energy_domains);
		}
	}
	ts = extlib_read_energy_measurements(&energy_domains);

	extlib_free_energy_timestamp(ts);

	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
		{
			std::string domain_name = getEnergyDomainName(domain);
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].size(), 3)
				<< "Size of container for reserved memory pages of " << domain_name << " energy domain should be 3.";
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].count(2), 1)
				<< "Reserved memory page 2 of " << domain_name << " energy domain is missing.";
		}
	}
	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 3) << "Size of metadata container should be 3.";
	EXPECT_EQ(ReservedMemoryManager::metadata.count(2), 1) << "Container for energy timestamp metadata for reserved memory page 2 is missing.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 3) << "Size of entry_length container should be 3.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.count(2), 1) << "Number of entries in reserved memory page 2 is missing.";
	EXPECT_EQ(ReservedMemoryManager::empty_page_ids.size(), 1) << "Size of empty_page_ids container should be 1.";
	EXPECT_EQ(ReservedMemoryManager::empty_page_ids.count(2), 1) << "Reserved memory page 2 is empty but its ID is missing in empty_page_ids countainer.";

	extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, ShrinkToFitRemovesEmptyReservedMemoryPageOfNonZeroIndex)
{
    ExtlibEnergy energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	ExtlibEnergyTimeStamp *ts_array[2][RESERVED_ELEMENT_SIZE * 2];
	int n_pages = 2;
	int n_ts = RESERVED_ELEMENT_SIZE * 2;
	for (int i = 0; i < n_pages; i++)
	{
		for (int j = 0; j < n_ts; j++)
		{
			ts_array[i][j] = extlib_read_energy_measurements(&energy_domains);
		}
	}
	ExtlibEnergyTimeStamp *ts = extlib_read_energy_measurements(&energy_domains);

	for (int i = 0; i < n_ts; i++)
	{
		extlib_free_energy_timestamp(ts_array[1][i]);
	}

	extlib_shrink_to_fit();

	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
		{
			std::string domain_name = getEnergyDomainName(domain);
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].size(), 2)
				<< "Size of container for reserved memory pages of " << domain_name << " energy domain should be 2"
				<< " after reserved memory page 1 has been removed.";
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].count(1), 0)
				<< "Reserved memory page 1 of " << domain_name << " energy domain should be removed.";
		}
	}
	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 2) << "Size of metadata container should be 2" << " after reserved memory page 1 has been removed.";
	EXPECT_EQ(ReservedMemoryManager::metadata.count(1), 0)
		<< "Container for energy timestamp metadata for reserved memory page 1 should be removed" << " after the page has been removed.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 2)
		<< "Size of entry_length container should be 2" << " after reserved memory page 1 has been removed.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.count(1), 0)
		<< "Number of entries in reserved memory page 1 should be removed" << " after the page has been removed.";
	EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.empty())
		<< "empty_page_ids container should be empty" << " after reserved memory page 1 has been removed.";

	extlib_close(&energy_domains);

	energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	for (int i = 0; i < n_pages; i++)
	{
		for (int j = 0; j < n_ts; j++)
		{
			ts_array[i][j] = extlib_read_energy_measurements(&energy_domains);
		}
	}
	ts = extlib_read_energy_measurements(&energy_domains);

	extlib_free_energy_timestamp(ts);

	extlib_shrink_to_fit();

	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
		{
			std::string domain_name = getEnergyDomainName(domain);
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].size(), 2)
				<< "Size of container for reserved memory pages of " << domain_name << " energy domain should be 2"
				<< " after reserved memory page 2 has been removed.";
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].count(2), 0)
				<< "Reserved memory page 2 of " << domain_name << " energy domain should be removed.";
		}
	}
	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 2) << "Size of metadata container should be 2" << " after reserved memory page 2 has been removed.";
	EXPECT_EQ(ReservedMemoryManager::metadata.count(2), 0)
		<< "Container for energy timestamp metadata for reserved memory page 2 should be removed" << " after the page has been removed.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 2)
		<< "Size of entry_length container should be 2" << " after reserved memory page 2 has been removed.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.count(2), 0)
		<< "Number of entries in reserved memory page 2 should be removed" << " after the page has been removed.";
	EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.empty())
		<< "empty_page_ids container should be empty" << " after reserved memory page 2 has been removed.";

	extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, EmptyReservedMemoryPage0IsNeverRemoved)
{
    ExtlibEnergy energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	ExtlibEnergyTimeStamp *ts_array[RESERVED_ELEMENT_SIZE * 2];
	int n_ts = RESERVED_ELEMENT_SIZE * 2;
	for (int i = 0; i < n_ts; i++)
	{
		ts_array[i] = extlib_read_energy_measurements(&energy_domains);
	}
	ExtlibEnergyTimeStamp *ts = extlib_read_energy_measurements(&energy_domains);

	for (int i = 0; i < n_ts; i++)
	{
		extlib_free_energy_timestamp(ts_array[i]);
	}

	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
		{
			std::string domain_name = getEnergyDomainName(domain);
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].size(), 2)
				<< "Size of container for reserved memory pages of " << domain_name << " energy domain should be 2.";
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].count(0), 1)
				<< "Reserved memory page 0 of " << domain_name << " energy domain is missing.";
		}
	}
	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 2) << "Size of metadata container should be 2.";
	EXPECT_EQ(ReservedMemoryManager::metadata.count(0), 1) << "Container for energy timestamp metadata for reserved memory page 0 is missing.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 2) << "Size of entry_length container should be 2.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.count(0), 1) << "Number of entries in reserved memory page 0 is missing.";
	EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.empty()) << "empty_page_ids container should be empty.";

	extlib_shrink_to_fit();

	for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
	{
		unsigned int domain = 1 << domain_idx;
		if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
		{
			std::string domain_name = getEnergyDomainName(domain);
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].size(), 2)
				<< "Size of container for reserved memory pages of " << domain_name << " energy domain should be 2.";
			EXPECT_EQ(ReservedMemoryManager::domain_memory_pool[domain_idx].count(0), 1)
				<< "Reserved memory page 0 of " << domain_name << " energy domain is missing.";
		}
	}
	EXPECT_EQ(ReservedMemoryManager::metadata.size(), 2) << "Size of metadata container should be 2.";
	EXPECT_EQ(ReservedMemoryManager::metadata.count(0), 1) << "Container for energy timestamp metadata for reserved memory page 0 is missing.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.size(), 2) << "Size of entry_length container should be 2.";
	EXPECT_EQ(ReservedMemoryManager::entry_length.count(0), 1) << "Number of entries in reserved memory page 0 is missing.";
	EXPECT_TRUE(ReservedMemoryManager::empty_page_ids.empty()) << "empty_page_ids container should be empty.";

	extlib_close(&energy_domains);
}

TEST(MemoryManagerTest, PrintEnergyTimestampPrintsEnergyTimeStamp)
{
    ExtlibEnergy energy_domains = {};
	EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, RESERVED_ELEMENT_SIZE);
	enableAllEnergyDomains(energy_domains);
	extlib_init(&energy_domains, true);

	ExtlibEnergyTimeStamp *ts_array[2][RESERVED_ELEMENT_SIZE * 2];
	int n_ts = RESERVED_ELEMENT_SIZE * 2;
	for (int i = 0; i < n_ts; i++)
	{
		ts_array[0][i] = extlib_read_energy_measurements(&energy_domains);
	}
	do_work(2);
	for (int i = 0; i < n_ts; i++)
	{
		ts_array[1][i] = extlib_read_energy_measurements(&energy_domains);
	}
	auto *result = extlib_calc_energy_consumption(ts_array[0][0], ts_array[1][0]);

	std::cout << "\nPrinting energy timestamp created in reserved memory page 0...\n\n";
	extlib_print_energy_timestamp(ts_array[0][0], -1);

	std::cout << "\nPrinting energy timestamp created in reserved memory page 1...\n\n";
	extlib_print_energy_timestamp(ts_array[1][0], -1);

	std::cout << "\nPrinting result energy timestamp...\n\n";
	extlib_print_energy_timestamp(result, -1);

	extlib_close(&energy_domains);
}
