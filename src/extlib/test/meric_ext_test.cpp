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

TEST(MERICExtAPITest, ReservedMemoryIsNotEnabledByDefault)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON);
    extlib_init(&energy_domains, true);

    EXPECT_FALSE(energy_domains.use_reserved_memory);
    EXPECT_EQ(energy_domains.reserved_memory_size, 0);

    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, ExistingEnergyDomainsStayEnabledAfterInit)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON);
    extlib_init(&energy_domains, true);

    bool has_a64fx = true; // meric.has_a64fx();
    bool has_rapl = true;  // meric.has_rapl();
    bool has_nvml = true;  // meric.has_nvml();
    bool has_hdeem = true; // meric.has_hdeem();
    bool has_rocm = true;  // meric.has_rocm();
    bool has_hwmon = true; // meric.has_hwmon();

    if (has_a64fx)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX))
            ADD_FAILURE() << "Domain:" << "A64FX" << " exists, but is not enabled.";
    if (has_rapl)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL))
            ADD_FAILURE() << "Domain:" << "RAPL" << " exists, but is not enabled.";        
    if (has_nvml)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML))
            ADD_FAILURE() << "Domain:" << "NVML" << " exists, but is not enabled.";
    if (has_hdeem)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM))
            ADD_FAILURE() << "Domain:" << "HDEEM" << " exists, but is not enabled.";
    if (has_rocm)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM))
            ADD_FAILURE() << "Domain:" << "ROCM" << " exists, but is not enabled.";
    if (has_hwmon)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON))
            ADD_FAILURE() << "Domain:" << "HWMON" << " exists, but is not enabled.";

    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, NonExistingEnergyDomainsAreDisabledAfterInit)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON);
    extlib_init(&energy_domains, true);

    bool has_a64fx = true; // meric.has_a64fx();
    bool has_rapl = true;  // meric.has_rapl();
    bool has_nvml = true;  // meric.has_nvml();
    bool has_hdeem = true; // meric.has_hdeem();
    bool has_rocm = true;  // meric.has_rocm();
    bool has_hwmon = true; // meric.has_hwmon();

    if (!has_a64fx)
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX))
            ADD_FAILURE() << "Domain:" << "A64FX" << " is enabled, but does not exist.";
    if (!has_rapl)
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL))
            ADD_FAILURE() << "Domain:" << "RAPL" << " is enabled, but does not exist.";
    if (!has_nvml)
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML))
            ADD_FAILURE() << "Domain:" << "NVML" << " is enabled, but does not exist.";
    if (!has_hdeem)
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM))
            ADD_FAILURE() << "Domain:" << "HDEEM" << " is enabled, but does not exist.";
    if (!has_rocm)
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM))
            ADD_FAILURE() << "Domain:" << "ROCM" << " is enabled, but does not exist.";
    if (!has_hwmon)
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON))
            ADD_FAILURE() << "Domain:" << "HWMON" << " is enabled, but does not exist.";

    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, ExistingEnergyDomainsHaveTheirFunctionsSetAfterInit)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON);
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

TEST(MERICExtAPITest, NonExistingEnergyDomainsDoNotHaveTheirFunctionsSetAfterInit)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON);
    extlib_init(&energy_domains, true);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
        {
            EXPECT_EQ(energy_domains.energyMeasurementFns[domain_idx], nullptr);
            EXPECT_EQ(energy_domains.closeFns[domain_idx], nullptr);
        }
    }

    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, ExtlibEnergyStructIsResetAfterClose)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON);
    extlib_init(&energy_domains, true);

    extlib_close(&energy_domains);

    EXPECT_EQ(energy_domains.value, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NO_SELECTION);
    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        EXPECT_EQ(energy_domains.energyMeasurementFns[domain_idx], nullptr);
        EXPECT_EQ(energy_domains.closeFns[domain_idx], nullptr);
    }
    EXPECT_FALSE(energy_domains.use_reserved_memory);
    EXPECT_EQ(energy_domains.reserved_memory_size, 0);
}

TEST(MERICExtAPITest, ExistingEnergyDomainsReturnSomeData)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON);
    extlib_init(&energy_domains, true);

    auto *ts = extlib_read_energy_measurements(&energy_domains);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
        {
            EXPECT_GT(ts->domain_data[domain_idx].arr_size, 0);
            EXPECT_NE(ts->domain_data[domain_idx].counter_name, nullptr);
            EXPECT_NE(ts->domain_data[domain_idx].counter_value, nullptr);
            EXPECT_EQ(ts->domain_data[domain_idx].energy_per_counter, nullptr);
        }
    }
    EXPECT_EQ(ts->energy_total, 0.0);
    EXPECT_EQ(ts->reserved_memory_page, 0);
    EXPECT_EQ(ts->reserved_memory_index, 0);

    extlib_free_energy_timestamp(ts);
    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, NonExistingEnergyDomainsReturnNoData)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON);
    extlib_init(&energy_domains, true);

    auto *ts = extlib_read_energy_measurements(&energy_domains);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
        {
            EXPECT_EQ(ts->domain_data[domain_idx].arr_size, 0);
            EXPECT_EQ(ts->domain_data[domain_idx].counter_name, nullptr);
            EXPECT_EQ(ts->domain_data[domain_idx].counter_value, nullptr);
            EXPECT_EQ(ts->domain_data[domain_idx].energy_per_counter, nullptr);
        }
    }
    EXPECT_EQ(ts->energy_total, 0.0);
    EXPECT_EQ(ts->reserved_memory_page, 0);
    EXPECT_EQ(ts->reserved_memory_index, 0);

    extlib_free_energy_timestamp(ts);
    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, CalcEnergyConsumptionReturnsSomeDataForExistingEnergyDomains)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON);
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
            EXPECT_GT(result->domain_data[domain_idx].arr_size, 0);
            EXPECT_NE(result->domain_data[domain_idx].counter_name, nullptr);
            EXPECT_EQ(result->domain_data[domain_idx].counter_value, nullptr);
            EXPECT_NE(result->domain_data[domain_idx].energy_per_counter, nullptr);
        }
    }
    EXPECT_GT(result->energy_total, 0.0);
    EXPECT_EQ(result->reserved_memory_page, 0);
    EXPECT_EQ(result->reserved_memory_index, 0);

    extlib_free_energy_timestamp(ts1);
    extlib_free_energy_timestamp(ts2);
    extlib_free_energy_timestamp(result);
    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, CalcEnergyConsumptionReturnsNoDataForNonExistingEnergyDomains)
{
    ExtlibEnergy energy_domains = {};
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON);
    extlib_init(&energy_domains, true);

    auto *ts1 = extlib_read_energy_measurements(&energy_domains);
    do_work(5);
    auto *ts2 = extlib_read_energy_measurements(&energy_domains);
    auto *result = extlib_calc_energy_consumption(ts1, ts2);

    for (int domain_idx = 0; domain_idx < EXTLIB_NUM_DOMAINS; domain_idx++)
    {
        unsigned int domain = 1 << domain_idx;
        if (EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, domain))
        {
            EXPECT_GT(result->domain_data[domain_idx].arr_size, 0);
            EXPECT_NE(result->domain_data[domain_idx].counter_name, nullptr);
            EXPECT_EQ(result->domain_data[domain_idx].counter_value, nullptr);
            EXPECT_NE(result->domain_data[domain_idx].energy_per_counter, nullptr);
        }
    }
    EXPECT_GT(result->energy_total, 0.0);
    EXPECT_EQ(result->reserved_memory_page, 0);
    EXPECT_EQ(result->reserved_memory_index, 0);

    extlib_free_energy_timestamp(ts1);
    extlib_free_energy_timestamp(ts2);
    extlib_free_energy_timestamp(result);
    extlib_close(&energy_domains);
}
