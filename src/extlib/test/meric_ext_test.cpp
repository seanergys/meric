// =============================================================================
// MERICExt API Tests
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

TEST(MERICExtAPITest, NumberOfEnergyDomainsAndTheirIdsAreSetCorrectly)
{
    int expected_num_domains = 6;
    ASSERT_EQ(EXTLIB_NUM_DOMAINS, expected_num_domains)
        << "EXTLIB_NUM_DOMAINS macro should be " << expected_num_domains
        << " (" << EXTLIB_NUM_DOMAINS << " != " << expected_num_domains << ").";

    unsigned int domain_ids[EXTLIB_NUM_DOMAINS] = {ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX,
                                                ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL,
                                                ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML,
                                                ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM,
                                                ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM,
                                                ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON};

    std::string domain_names[EXTLIB_NUM_DOMAINS] = {"A64FX", "RAPL", "NVML", "ROCM", "HDEEM", "HWMON"};

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int expected_domain_id = 1 << domain_idx;
        EXPECT_EQ(domain_ids[domain_idx], expected_domain_id)
            << "ID of " << domain_names[domain_idx] << " energy domain should be " << expected_domain_id
            << " (" << domain_ids[domain_idx] << " != " << expected_domain_id << ").";
    }
}

TEST(MERICExtAPITest, ReservedMemoryIsDisabledByDefault)
{
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    EXPECT_FALSE(ReservedMemoryManager::is_enabled)
        << "is_enabled boolean should be false.";
    EXPECT_EQ(ReservedMemoryManager::reserved_memory_size, 0)
        << "reserved_memory_size variable should be 0.";

    EXPECT_FALSE(energy_domains.use_reserved_memory)
        << "use_reserved_memory boolean should be false.";
    EXPECT_EQ(energy_domains.reserved_memory_size, 0)
        << "reserved_memory_size variable should be 0.";

    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, EnabledExistingEnergyDomainsStayEnabledAfterInit)
{
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    bool has_a64fx = true; // meric.has_a64fx();
    bool has_rapl = true;  // meric.has_rapl();
    bool has_nvml = true;  // meric.has_nvml();
    bool has_hdeem = true; // meric.has_hdeem();
    bool has_rocm = true;  // meric.has_rocm();
    bool has_hwmon = true; // meric.has_hwmon();

    if (has_a64fx)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX))
            ADD_FAILURE() << "A64FX energy domain exists but is not enabled.";
    if (has_rapl)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL))
            ADD_FAILURE() << "RAPL energy domain exists but is not enabled.";        
    if (has_nvml)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML))
            ADD_FAILURE() << "NVML energy domain exists but is not enabled.";
    if (has_hdeem)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM))
            ADD_FAILURE() << "HDEEM energy domain exists but is not enabled.";
    if (has_rocm)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM))
            ADD_FAILURE() << "ROCM energy domain exists but is not enabled.";
    if (has_hwmon)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON))
            ADD_FAILURE() << "HWMON energy domain exists but is not enabled.";

    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, EnabledNonExistingEnergyDomainsAreDisabledAfterInit)
{
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    bool has_a64fx = true; // meric.has_a64fx();
    bool has_rapl = true;  // meric.has_rapl();
    bool has_nvml = true;  // meric.has_nvml();
    bool has_hdeem = true; // meric.has_hdeem();
    bool has_rocm = true;  // meric.has_rocm();
    bool has_hwmon = true; // meric.has_hwmon();

    if (!has_a64fx)
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX))
            ADD_FAILURE() << "A64FX energy domain does not exist but is enabled";
    if (!has_rapl)
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL))
            ADD_FAILURE() << "RAPL energy domain does not exist but is enabled";
    if (!has_nvml)
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML))
            ADD_FAILURE() << "NVML energy domain does not exist but is enabled";
    if (!has_hdeem)
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM))
            ADD_FAILURE() << "HDEEM energy domain does not exist but is enabled";
    if (!has_rocm)
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM))
            ADD_FAILURE() << "ROCM energy domain does not exist but is enabled";
    if (!has_hwmon)
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON))
            ADD_FAILURE() << "HWMON energy domain does not exist but is enabled";

    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, DisabledEnergyDomainsStayDisabledAfterInit)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_NO_SELECTION(energy_domains);
    extlib_init(&energy_domains, true);

    for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
        {
            std::string domain_name = getEnergyDomainName(domain);
            ADD_FAILURE() << domain_name << " energy domain should be disabled.";
        }
    }

    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, EnabledExistingEnergyDomainsHaveTheirFunctionsSetAfterInit)
{
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
        {
            std::string domain_name = getEnergyDomainName(domain);
            EXPECT_NE(energy_domains.energyMeasurementFns[domain_idx], nullptr)
                << domain_name << " energy domain exists but its energy measurement function is not set.";
            EXPECT_NE(energy_domains.closeFns[domain_idx], nullptr)
                << domain_name << " energy domain exists but its close function is not set.";
        }
    }

    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, EnabledNonExistingEnergyDomainsDoNotHaveTheirFunctionsSetAfterInit)
{
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
        {
            std::string domain_name = getEnergyDomainName(domain);
            EXPECT_EQ(energy_domains.energyMeasurementFns[domain_idx], nullptr)
                << domain_name << " energy domain does not exist but its energy measurement function is set.";
            EXPECT_EQ(energy_domains.closeFns[domain_idx], nullptr)
                << domain_name << " energy domain does not exist but its close function is set.";
        }
    }

    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, DisabledEnergyDomainsDoNotHaveTheirFunctionsSetAfterInit)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_NO_SELECTION(energy_domains);
    extlib_init(&energy_domains, true);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        std::string domain_name = getEnergyDomainName(domain);
        EXPECT_EQ(energy_domains.energyMeasurementFns[domain_idx], nullptr)
            << domain_name << " energy domain is disabled but its energy measurement function is set.";
        EXPECT_EQ(energy_domains.closeFns[domain_idx], nullptr)
            << domain_name << " energy domain is disabled but its close function is set.";
    }

    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, EnergyDomainsAreInResetStateAfterClose)
{
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    extlib_close(&energy_domains);

    EXPECT_EQ(energy_domains.value, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NO_SELECTION)
        << "All energy domains should be disabled.";
    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        std::string domain_name = getEnergyDomainName(domain);
        EXPECT_EQ(energy_domains.energyMeasurementFns[domain_idx], nullptr)
            << "Energy measurement function of " << domain_name << " energy domain should not be set.";
        EXPECT_EQ(energy_domains.closeFns[domain_idx], nullptr)
            << "Close function of " << domain_name << " energy domain should not be set.";
    }
    EXPECT_FALSE(energy_domains.use_reserved_memory)
        << "use_reserved_memory boolean should be false.";
    EXPECT_EQ(energy_domains.reserved_memory_size, 0)
        << "reserved_memory_size variable should be 0.";
}

TEST(MERICExtAPITest, SuccessivelyCreatedEnergyTimeStampsHaveSuccessiveNonZeroIds)
{
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    auto *ts1 = extlib_read_energy_measurements(&energy_domains);
    do_work(2);
    auto *ts2 = extlib_read_energy_measurements(&energy_domains);
    auto *result = extlib_calc_energy_consumption(ts1, ts2);

    EXPECT_GT(ts1->id, 0.0)
        << "ID of the first energy timestamp (ts1) is 0.";
    EXPECT_GT(ts2->id, ts1->id)
        << "ID of the second energy timestamp (ts2) is lesser than ID of the first one (ts1).";
    EXPECT_GT(result->id, ts2->id)
        << "ID of the third energy timestamp (result) is lesser than ID of the second one (ts2).";

    extlib_free_energy_timestamp(ts1);
    extlib_free_energy_timestamp(ts2);
    extlib_free_energy_timestamp(result);
    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, EnergyTimeStampsHaveReservedMemoryVariablesInDefaultState)
{
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    auto *ts1 = extlib_read_energy_measurements(&energy_domains);
    do_work(2);
    auto *ts2 = extlib_read_energy_measurements(&energy_domains);

    EXPECT_EQ(ts1->reserved_memory_page, 0)
        << "reserved_memory_page variable should be 0.";
    EXPECT_EQ(ts1->reserved_memory_index, 0)
        << "reserved_memory_index variable should be 0.";
    EXPECT_EQ(ts2->reserved_memory_page, 0)
        << "reserved_memory_page variable should be 0.";
    EXPECT_EQ(ts2->reserved_memory_index, 0)
        << "reserved_memory_index variable should be 0.";

    auto *ts3 = extlib_read_energy_measurements(&energy_domains);
    do_work(2);
    auto *ts4 = extlib_read_energy_measurements(&energy_domains);

    EXPECT_EQ(ts3->reserved_memory_page, 0)
        << "reserved_memory_page variable should be 0.";
    EXPECT_EQ(ts3->reserved_memory_index, 0)
        << "reserved_memory_index variable should be 0.";
    EXPECT_EQ(ts4->reserved_memory_page, 0)
        << "reserved_memory_page variable should be 0.";
    EXPECT_EQ(ts4->reserved_memory_index, 0)
        << "reserved_memory_index variable should be 0.";

    auto *result1 = extlib_calc_energy_consumption(ts1, ts2);
    auto *result2 = extlib_calc_energy_consumption(ts3, ts4);

    EXPECT_EQ(result1->reserved_memory_page, 0)
        << "reserved_memory_page variable should be 0.";
    EXPECT_EQ(result1->reserved_memory_index, 0)
        << "reserved_memory_index variable should be 0.";
    EXPECT_EQ(result2->reserved_memory_page, 0)
        << "reserved_memory_page variable should be 0.";
    EXPECT_EQ(result2->reserved_memory_index, 0)
        << "reserved_memory_index variable should be 0.";

    extlib_free_energy_timestamp(ts1);
    extlib_free_energy_timestamp(ts2);
    extlib_free_energy_timestamp(ts3);
    extlib_free_energy_timestamp(ts4);
    extlib_free_energy_timestamp(result1);
    extlib_free_energy_timestamp(result2);
    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, ReadEnergyMeasurementsReturnsEnergyTimeStampWithCorrectlySetEnergyDomainIds)
{
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    auto *ts = extlib_read_energy_measurements(&energy_domains);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int expected_domain_id = 1 << domain_idx;
        std::string domain_name = getEnergyDomainName(expected_domain_id);
        EXPECT_EQ(ts->domain_data[domain_idx].domain_id, expected_domain_id)
            << "ID of " << domain_name << " energy domain should be " << expected_domain_id
            << " (" << (unsigned int)ts->domain_data[domain_idx].domain_id << " != " << expected_domain_id << ").";
    }

    extlib_free_energy_timestamp(ts);
    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, CalcEnergyConsumptionReturnsEnergyTimeStampWithCorrectlySetEnergyDomainIds)
{
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    auto *ts1 = extlib_read_energy_measurements(&energy_domains);
    do_work(5);
    auto *ts2 = extlib_read_energy_measurements(&energy_domains);
    auto *result = extlib_calc_energy_consumption(ts1, ts2);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int expected_domain_id = 1 << domain_idx;
        std::string domain_name = getEnergyDomainName(expected_domain_id);
        EXPECT_EQ(result->domain_data[domain_idx].domain_id, expected_domain_id)
            << "ID of " << domain_name << " energy domain should be " << expected_domain_id
            << " (" << (unsigned int)result->domain_data[domain_idx].domain_id << " != " << expected_domain_id << ").";
    }

    extlib_free_energy_timestamp(ts1);
    extlib_free_energy_timestamp(ts2);
    extlib_free_energy_timestamp(result);
    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, ReadEnergyMeasurementsReturnsSomeDataForExistingEnergyDomains)
{
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    auto *ts = extlib_read_energy_measurements(&energy_domains);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
        {
            std::string domain_name = getEnergyDomainName(domain);
            EXPECT_GT(ts->domain_data[domain_idx].arr_size, 0)
                << domain_name << " energy domain exists but its arr_size variable is 0.";
            EXPECT_NE(ts->domain_data[domain_idx].counter_name, nullptr)
                << domain_name << " energy domain exists but its counter_name pointer is NULL/nullptr.";
            EXPECT_NE(ts->domain_data[domain_idx].counter_value, nullptr)
                << domain_name << " energy domain exists but its counter_value pointer is NULL/nullptr.";
            EXPECT_EQ(ts->domain_data[domain_idx].energy_per_counter, nullptr)
                << "energy_per_counter pointer of " << domain_name << " energy domain should be NULL/nullptr.";
            EXPECT_EQ(ts->domain_data[domain_idx].energy_total, 0.0)
                << "energy_total variable of " << domain_name << " energy domain should be 0.";
        }
    }
    EXPECT_EQ(ts->energy_total, 0.0)
        << "energy_total variable should be 0.";

    extlib_free_energy_timestamp(ts);
    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, ReadEnergyMeasurementsReturnsNoDataForNonExistingEnergyDomains)
{
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    auto *ts = extlib_read_energy_measurements(&energy_domains);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
        {
            std::string domain_name = getEnergyDomainName(domain);
            EXPECT_EQ(ts->domain_data[domain_idx].arr_size, 0)
                << domain_name << " energy domain does not exist but its arr_size variable is greater than 0.";
            EXPECT_EQ(ts->domain_data[domain_idx].counter_name, nullptr)
                << domain_name << " energy domain does not exist but its counter_name pointer is not NULL/nullptr.";
            EXPECT_EQ(ts->domain_data[domain_idx].counter_value, nullptr)
                << domain_name << " energy domain does not exist but its counter_value pointer is not NULL/nullptr.";
            EXPECT_EQ(ts->domain_data[domain_idx].energy_per_counter, nullptr)
                << "energy_per_counter pointer of " << domain_name << " energy domain should be NULL/nullptr.";
            EXPECT_EQ(ts->domain_data[domain_idx].energy_total, 0.0)
                << "energy_total variable of " << domain_name << " energy domain should be 0.";
        }
    }
    EXPECT_EQ(ts->energy_total, 0.0)
        << "energy_total variable should be 0.";

    extlib_free_energy_timestamp(ts);
    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, CalcEnergyConsumptionReturnsSomeDataForExistingEnergyDomains)
{
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    auto *ts1 = extlib_read_energy_measurements(&energy_domains);
    do_work(5);
    auto *ts2 = extlib_read_energy_measurements(&energy_domains);
    auto *result = extlib_calc_energy_consumption(ts1, ts2);

    extlib_print_energy_timestamp(ts1, -1);
    extlib_print_energy_timestamp(ts2, -1);
    extlib_print_energy_timestamp(result, -1);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
        {
            std::string domain_name = getEnergyDomainName(domain);
            EXPECT_GT(result->domain_data[domain_idx].arr_size, 0)
                << domain_name << " energy domain exists but its arr_size variable is 0.";
            EXPECT_NE(result->domain_data[domain_idx].counter_name, nullptr)
                << domain_name << " energy domain exists but its counter_name pointer is NULL/nullptr.";
            EXPECT_EQ(result->domain_data[domain_idx].counter_value, nullptr)
                << "counter_value pointer of " << domain_name << " energy domain should be NULL/nullptr.";
            EXPECT_NE(result->domain_data[domain_idx].energy_per_counter, nullptr)
                << domain_name << " energy domain exists but its energy_per_counter pointer is NULL/nullptr.";
            EXPECT_GT(result->domain_data[domain_idx].energy_total, 0.0)
                << domain_name << " energy domain exists but its energy_total variable is 0.";
        }
    }
    EXPECT_GT(result->energy_total, 0.0)
        << "energy_total variable should be greater than 0.";

    extlib_free_energy_timestamp(ts1);
    extlib_free_energy_timestamp(ts2);
    extlib_free_energy_timestamp(result);
    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, CalcEnergyConsumptionReturnsNoDataForNonExistingEnergyDomains)
{
    ExtlibEnergy energy_domains = {};
    enableAllEnergyDomains(energy_domains);
    extlib_init(&energy_domains, true);

    auto *ts1 = extlib_read_energy_measurements(&energy_domains);
    do_work(5);
    auto *ts2 = extlib_read_energy_measurements(&energy_domains);
    auto *result = extlib_calc_energy_consumption(ts1, ts2);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
        {
            std::string domain_name = getEnergyDomainName(domain);
            EXPECT_EQ(result->domain_data[domain_idx].arr_size, 0)
                << domain_name << " energy domain does not exist but its arr_size variable is greater than 0.";
            EXPECT_EQ(result->domain_data[domain_idx].counter_name, nullptr)
                << domain_name << " energy domain does not exist but its counter_name pointer is not NULL/nullptr.";
            EXPECT_EQ(result->domain_data[domain_idx].counter_value, nullptr)
                << "counter_value pointer of " << domain_name << " energy domain should be NULL/nullptr.";
            EXPECT_EQ(result->domain_data[domain_idx].energy_per_counter, nullptr)
                << domain_name << " energy domain does not exist but its energy_per_counter pointer is not NULL/nullptr.";
            EXPECT_EQ(result->domain_data[domain_idx].energy_total, 0.0)
                << domain_name << " energy domain does not exist but its energy_total variable is greater than 0.";
        }
    }

    extlib_free_energy_timestamp(ts1);
    extlib_free_energy_timestamp(ts2);
    extlib_free_energy_timestamp(result);
    extlib_close(&energy_domains);
}
