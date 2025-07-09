// ====================================================================
// MERIC ext - library directly exposing MERIC internal implementation
// --------------------------------------------------------------------
// Library providing energy measurement for 3rd party tools
//
// Author: Osman Yasal <osman.yasal@vsb.cz>
// Version: 1.0.0
// License: BSD-3
// ====================================================================

#ifndef SRC_EXTLIB_MERIC_EXT_H_
#define SRC_EXTLIB_MERIC_EXT_H_

#include <stdbool.h>
#include <stdint.h> // Include this header for uint64_t
#include <stdio.h>  // If you also need I/O functions

#define EXTLIB_NUM_DOMAINS 6 // TODO <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
struct ExtlibEnergyTimeStamp
{
    double id; // Timestamp identifier, also implies if the memory location in static_memory is available or not. if 0, it is available for an entry otherwise is NOT.

    // Embedded DomainData structure for each domain (a64fx, rapl, nvml, rocm, hdeem)
    struct
    {
        uint8_t domain_id;             // EXTLIB_ENERGY_DOMAIN_{A64FX, RAPL, NVML, ROCM, HDEEM...}
        size_t arr_size;               // Number of counters in this domain
        char **counter_name;           // Array of counter names
        uint64_t *counter_value;       // Array of raw counter values
        double *energy_per_counter;    // Energy per counter (joules)
        double energy_total;           // Total energy for this domain
    } domain_data[EXTLIB_NUM_DOMAINS]; // Array per domain (indexed by mutils::countTrailingZeros)

    double energy_total; // Combined total energy across all domains

    size_t reserved_memory_page;  // used to store the size of static_memory.
    size_t reserved_memory_index; // used to store the location in static_memory.
};

typedef struct ExtlibEnergy
{
    enum Domains // TODO HDEEM-C vs HDEEM-S <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    {
        EXTLIB_ENERGY_DOMAIN_NO_SELECTION = 0, // 0000000 No Domains Selected
        EXTLIB_ENERGY_DOMAIN_A64FX = 1 << 0,   // 0000001
        EXTLIB_ENERGY_DOMAIN_RAPL = 1 << 1,    // 0000010
        EXTLIB_ENERGY_DOMAIN_NVML = 1 << 2,    // 0000100
        EXTLIB_ENERGY_DOMAIN_ROCM = 1 << 3,    // 0001000
        EXTLIB_ENERGY_DOMAIN_HDEEM = 1 << 4,   // 0010000
        EXTLIB_ENERGY_DOMAIN_HWMON = 1 << 5,   // 0100000
        EXTLIB_ENERGY_DOMAIN_END = 1 << 6      // 1000000 END of Domains.
    };
    unsigned int value;

    void (*energyMeasurementFns[EXTLIB_NUM_DOMAINS])(size_t *, uint64_t **, char ***, size_t, size_t, size_t); // Function pointers to energy measurements for enabled domains
    void (*closeFns[EXTLIB_NUM_DOMAINS])();                                                                    // Function pointer to close energy measurement settings for enabled domains
    bool use_reserved_memory;
    size_t reserved_memory_size;

} ExtlibEnergy;

// Helper macros for bitwise operations

// To measure energy consumption, you need 2 EnergyTimeStamp and +1 created when returning from extlib_calc_energy_consumption(..)
// So in total, you need to allocate (2x + x) = 3x elements where x-> energy measurement count
#define EXTLIB_ENERGY_USE_RESERVED_MEMORY(enabled_domains, total_measurements) \
    (enabled_domains).use_reserved_memory = true;                              \
    (enabled_domains).reserved_memory_size = 3 * total_measurements

#define EXTLIB_ENERGY_ENABLE_DOMAIN(enabled_domains, domain) ((enabled_domains).value |= (domain))
#define EXTLIB_ENERGY_DISABLE_DOMAIN(enabled_domains, domain) ((enabled_domains).value &= ~(domain))
#define EXTLIB_ENERGY_HAS_DOMAIN(enabled_domains, domain) (((enabled_domains).value & (domain)) != 0)
#ifdef __cplusplus
#define EXTLIB_ENERGY_NO_SELECTION(enabled_domains) ((enabled_domains).value == ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NO_SELECTION)
#else
#define EXTLIB_ENERGY_NO_SELECTION(enabled_domains) ((enabled_domains).value == EXTLIB_ENERGY_DOMAIN_NO_SELECTION)
#endif

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

/**
 *
 * @param enabled_domains Object representing the enabled domains, used to initialize energy readings for the selected domains.
 * @param is_detailed Passed to the initialization methods of the selected domains to enable extra information output.
 *
 * @brief Initializes the required settings to monitor the selected domains.
 *
 */
EXTERNC void extlib_init(ExtlibEnergy *enabled_domains, bool is_detailed);

/**
 * @param enabled_domains Object representing the enabled domains, used to properly close them.
 *
 * @brief Pass the same object used in `extlib_init` to ensure correct closure of the domains.
 */
EXTERNC void extlib_close(ExtlibEnergy *enabled_domains);

/**
 * @param enabled_domains The object containing enabled domains, used to fetch energy readings from the selected domains.
 *
 * @brief Reads energy measurements using the `extlib_read_energy_measurements` function.
 *
 * This function returns an `EnergyTimeStamp` object containing energy counter readings
 * for all enabled and existing energy measurement domains.
 * You need to get another EnergyTimeStamp and call extlib_calc_energy_consumption(...) to calculate *total_energy fields
 *
 * You can print the retrieved `EnergyTimeStamp` object using:
 *
 * @code
 * extlib_print_energy_timestamp(ts, <number_of_values>);
 * @endcode
 *
 *
 *
 * EnergyTimeStamp Structure:
 * Each `EnergyTimeStamp` object holds energy readings for available domains and one total_energy field combining all.
 * Below is a sample output when the **A64Fx domain** is enabled and present in the test machine:
 *
 * ```
 * -------- ExtlibEnergyTimeStamp (id: 1738250797.6328) --------
 * | a64fx_counter_name (size: 56): [EA_CORE_00, EA_CORE_01, EA_CORE_02, EA_CORE_03, EA_CORE_04, ...]
 * | a64fx_counter_value (size: 56): [251544, 260996, 261868, 280186, 261523, ...]
 * | a64fx_energy_per_counter (size: 56): NULL or empty array
 * | a64fx_total_energy: 0.000000 J
 * |
 * | rapl_counter_name (size: 0): NULL or empty array
 * | rapl_counter_value (size: 0): NULL or empty array
 * | rapl_energy_per_counter (size: 0): NULL or empty array
 * | rapl_total_energy: 0.000000 J
 * |
 * | nvml_counter_name (size: 0): NULL or empty array
 * | nvml_counter_value (size: 0): NULL or empty array
 * | nvml_energy_per_counter (size: 0): NULL or empty array
 * | nvml_total_energy: 0.000000 J
 * |
 * | rocm_counter_name (size: 0): NULL or empty array
 * | rocm_counter_value (size: 0): NULL or empty array
 * | rocm_energy_per_counter (size: 0): NULL or empty array
 * | rocm_total_energy: 0.000000 J
 * |
 * | hdeem_counter_name (size: 0): NULL or empty array
 * | hdeem_counter_value (size: 0): NULL or empty array
 * | hdeem_energy_per_counter (size: 0): NULL or empty array
 * | hdeem_total_energy: 0.000000 J
 * |
 * | Total energy consumption: 0.000000 J
 * ------------------------------------------------------------
 * ```
 *
 * @note The `id` field represents the timestamp at which the object was created during energy measurement.
 *
 * @note The user of this method is responsible for freeing the timestamp, call extlib_free_energy_timestamp(...) to clean up.
 *
 */
EXTERNC struct ExtlibEnergyTimeStamp *extlib_read_energy_measurements(ExtlibEnergy *enabled_domains);

/**
 *
 * @param timestamp timestamp object to print
 * @param print_limit used to limit the array size to be printed, set -1 to see all
 *
 * @brief This method is used to print the ExtlibEnergyTimeStamp object,
 *
 */
EXTERNC void extlib_print_energy_timestamp(struct ExtlibEnergyTimeStamp *timestamp, size_t print_limit);

/**
 * @param timestamp timestamp object to be freed
 */
EXTERNC void extlib_free_energy_timestamp(struct ExtlibEnergyTimeStamp *timestamp);

/**
 * @param enabled_domains The object containing enabled domains
 *
 * @brief Returns enabled domains in a comma-separated format.
 *
 * @note The user of this method is responsible for freeing the memory.
 *
 */
EXTERNC char *extlib_get_enabled_domain_names(ExtlibEnergy *enabled_domains);

/**
 *
 * @param begin begin timestamp
 * @param end end timestamp
 *
 * @brief Both timestamps must be generated by the same ExtlibEnergy energy_domains object,
 *  The output of resulting timestamp is given below
 *
 * ```
 * -> you have the total energy consumption per domain ie. a64fx_total_energy, rapl_total_energy...
 * -> you also have the total energy consumption; Total_energy consumption: 1108.238159 [J], which combines all per domain energy.
 *
 *   ------- ExtlibEnergyTimeStamp (id: 1738250807.6350) -------
 *  | a64fx_counter_name (size: 56): [EA_CORE_00, EA_CORE_01, EA_CORE_02, EA_CORE_03, EA_CORE_04, ...]
 *  | a64fx_counter_value (size: 56): NULL or empty array
 *  | a64fx_energy_per_counter (size: 56): [20.003511, 22.578263, 20.002508, 20.511014, 20.002596, ...]
 *  | a64fx_total_energy: 1108.238159
 *  |
 *  | rapl_counter_name (size: 0): NULL or empty array
 *  | rapl_counter_value (size: 0): NULL or empty array
 *  | rapl_energy_per_counter (size: 0): NULL or empty array
 *  | rapl_total_energy: 0.000000
 *  |
 *  | nvml_counter_name (size: 0): NULL or empty array
 *  | nvml_counter_value (size: 0): NULL or empty array
 *  | nvml_energy_per_counter (size: 0): NULL or empty array
 *  | nvml_total_energy: 0.000000
 *  |
 *  | rocm_counter_name (size: 0): NULL or empty array
 *  | rocm_counter_value (size: 0): NULL or empty array
 *  | rocm_energy_per_counter (size: 0): NULL or empty array
 *  | rocm_total_energy: 0.000000
 *  |
 *  | hdeem_counter_name (size: 0): NULL or empty array
 *  | hdeem_counter_value (size: 0): NULL or empty array
 *  | hdeem_energy_per_counter (size: 0): NULL or empty array
 *  | hdeem_total_energy: 0.000000
 *  |
 *  | Total_energy consumption: 1108.238159 [J]
 *  -----------------------------------
 * ```
 *
 * @note The user of this method is responsible for freeing the timestamp, call extlib_free_energy_timestamp(...) to clean up.
 */
EXTERNC struct ExtlibEnergyTimeStamp *extlib_calc_energy_consumption(struct ExtlibEnergyTimeStamp *begin, struct ExtlibEnergyTimeStamp *end);

// undef region
#undef EXTERNC

#endif // SRC_EXTLIB_MERIC_EXT_H_
