#ifndef SRC_MIDLIB_MIDLIB_H_
#define SRC_MIDLIB_MIDLIB_H_

#include <stdbool.h>
#include <stdint.h> // Include this header for uint64_t

#ifdef HAVE_NVML_H
	#include <nvml.h>
#endif

#define MIDLIB_NUM_DOMAINS 5
struct MidlibEnergyTimeStamp
{
    double id; // Timestamp identifier

    // Embedded DomainData structure for each domain (a64fx, rapl, nvml, rocm, hdeem)
    struct
    {
        uint8_t domain_id;             // MIDLIB_ENERGY_DOMAIN_{A64FX, RAPL, NVML, ROCM, HDEEM...}
        size_t arr_size;               // Number of counters in this domain
        char **counter_name;           // Array of counter names
        uint64_t *counter_value;       // Array of raw counter values
        double *energy_per_counter;    // Energy per counter (joules)
        double energy_total;           // Total energy for this domain
    } domain_data[MIDLIB_NUM_DOMAINS]; // Array per domain (indexed by mutils::countTrailingZeros)

    double energy_total; // Combined total energy across all domains
};

typedef struct MidlibEnergy
{
    enum Domains
    {
        MIDLIB_ENERGY_DOMAIN_NO_SELECTION = 0, // 000000 No Domains Selected
        MIDLIB_ENERGY_DOMAIN_A64FX = 1 << 0,   // 000001
        MIDLIB_ENERGY_DOMAIN_RAPL = 1 << 1,    // 000010
        MIDLIB_ENERGY_DOMAIN_NVML = 1 << 2,    // 000100
        MIDLIB_ENERGY_DOMAIN_ROCM = 1 << 3,    // 001000
        MIDLIB_ENERGY_DOMAIN_HDEEM = 1 << 4,   // 010000
        MIDLIB_ENERGY_DOMAIN_END = 1 << 5      // 100000 END of Domains.
    };
    unsigned int value;

    void (*energyMeasurementFns[MIDLIB_NUM_DOMAINS])(size_t *, uint64_t **, char ***); // Function pointers to energy measurements for enabled domains
    void (*closeFns[MIDLIB_NUM_DOMAINS])();                                            // Close enabled domains

} MidlibEnergy;

// Helper macros for bitwise operations
#define MIDLIB_ENERGY_ENABLE_DOMAIN(enabled_domains, domain) ((enabled_domains).value |= (domain))
#define MIDLIB_ENERGY_DISABLE_DOMAIN(enabled_domains, domain) ((enabled_domains).value &= ~(domain))
#define MIDLIB_ENERGY_HAS_DOMAIN(enabled_domains, domain) (((enabled_domains).value & (domain)) != 0)
#ifdef __cplusplus
#define MIDLIB_ENERGY_NO_SELECTION(enabled_domains) ((enabled_domains).value == MidlibEnergy::MIDLIB_ENERGY_DOMAIN_NO_SELECTION)
#else
#define MIDLIB_ENERGY_NO_SELECTION(enabled_domains) ((enabled_domains).value == MIDLIB_ENERGY_DOMAIN_NO_SELECTION)
#endif

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC void midlib_init(MidlibEnergy *enabled_domains, bool is_detailed);
EXTERNC void midlib_close(MidlibEnergy *enabled_domains);

EXTERNC struct MidlibEnergyTimeStamp *midlib_read_energy_measurements(MidlibEnergy *enabled_domains);
EXTERNC void midlib_print_energy_timestamp(struct MidlibEnergyTimeStamp *timestamp, size_t print_limit);
EXTERNC void midlib_free_energy_timestamp(struct MidlibEnergyTimeStamp *timestamp);

EXTERNC char *midlib_get_enabled_domain_names(MidlibEnergy *enabled_domains); // NO NEED TO EXPOSE BUT I'LL KEEP IT HERE JUST IN CASE.

EXTERNC struct MidlibEnergyTimeStamp *midlib_calc_energy_consumption(struct MidlibEnergyTimeStamp *begin, struct MidlibEnergyTimeStamp *end);

// undef region
#undef EXTERNC

#endif // SRC_MIDLIB_MIDLIB_H_
