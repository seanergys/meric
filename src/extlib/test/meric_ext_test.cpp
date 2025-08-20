// =============================================================================
// MERICExt API Tests
//
// Author: Osman Yasal <osman.yasal@vsb.cz>
// Version: 1.0.0
// License: BSD-3
// =============================================================================

#include <thread>
#include <chrono>
#include <gtest/gtest.h>
#include "../meric_ext.h"
#include "../reserved_memory_manager.h"
#include "work.h"

TEST(MERICExtAPITest, ExistingEnergyDomainsSetUpCorrectly)
{
    ExtlibEnergy energy_domains = {0};
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
            ADD_FAILURE() << "Domain:" << "A64FX" << " exists but not correctly enabled.";

    if (has_rapl)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL))
            ADD_FAILURE() << "Domain:" << "RAPL" << " exists but not correctly enabled.";

    if (has_nvml)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML))
            ADD_FAILURE() << "Domain:" << "NVML" << " exists but not correctly enabled.";

    if (has_hdeem)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM))
            ADD_FAILURE() << "Domain:" << "HDEEM" << " exists but not correctly enabled.";

    if (has_rocm)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM))
            ADD_FAILURE() << "Domain:" << "ROCM" << " exists but not correctly enabled.";

    if (has_hwmon)
        if (!EXTLIB_ENERGY_HAS_DOMAIN(energy_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON))
            ADD_FAILURE() << "Domain:" << "HWMON" << " exists but not correctly enabled.";

    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, NonExistingDomainsAreClosedUp)
{
    // TODO: use EXTLIB_ENERGY_HAS_DOMAIN macro to check and ofc meric
}

TEST(MERICExtAPITest, ExistingDomainsReturnsSomeData)
{
    // TODO:call extlib_read_energy_measurements() and check if data returns.
}

TEST(MERICExtAPITest, CalcReturnsSomeData)
{
    ExtlibEnergy energy_domains = {0};
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

    // TODO:: check if result has data.

    extlib_close(&energy_domains);
}

TEST(MERICExtAPITest, MultipleInitCloseHasNoSideEffect)
{
    // Test body to be implemented
}
