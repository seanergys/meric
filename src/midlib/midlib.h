#ifndef SRC_MIDLIB_MIDLIB_H_
#define SRC_MIDLIB_MIDLIB_H_

#include <stdbool.h>

typedef struct MidlibEnergyDomains
{
    enum Domains
    {
        NO_SELECTION = 0, // 000000 No Domains Selected
        A64FX = 1 << 0,   // 000001
        RAPL = 1 << 1,    // 000010
        NVML = 1 << 2,    // 000100
        ROCM = 1 << 3,    // 001000
        HDEEM = 1 << 4,   // 010000
        END = 1 << 5      // 100000 END of Domains.
    };
    unsigned int value;

    void (*energyMeasurementFns[5])(bool, double);                                                           // Function pointers to energy measurements for enabled domains
    void (*printMeasurementFns[5])(bool);                                                                    // Function pointers to printing measurements for enabled domains
    void (*closeFns[5])();                                                                                   // Close enabled domains
    char *(*counterLabelFNs[5])(const char *);                                                               // Get counter labels for enabled domains
    // double (*getResultValueFNs[5])(unsigned long long, unsigned long long, double, int *, const char *); // Get counter labels for enabled domains

} MidlibEnergyDomains;

// Helper macros for bitwise operations
#define MIDLIB_ENERGY_ENABLE_DOMAIN(enabled_domains, domain) ((enabled_domains).value |= (domain))
#define MIDLIB_ENERGY_DISABLE_DOMAIN(enabled_domains, domain) ((enabled_domains).value &= ~(domain))
#define MIDLIB_ENERGY_HAS_DOMAIN(enabled_domains, domain) (((enabled_domains).value & (domain)) != 0)
#ifdef __cplusplus
#define MIDLIB_ENERGY_NO_SELECTION(enabled_domains) ((enabled_domains).value == MidlibEnergyDomains::NO_SELECTION)
#else
#define MIDLIB_ENERGY_NO_SELECTION(enabled_domains) ((enabled_domains).value == NO_SELECTION)
#endif

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC void midlib_init(MidlibEnergyDomains *enabled_domains, bool is_detailed);
EXTERNC void midlib_close(MidlibEnergyDomains *enabled_domains);

EXTERNC void midlib_read_energy_measurements(MidlibEnergyDomains *enabled_domains);
EXTERNC void midlib_print_energy_measurements(MidlibEnergyDomains *enabled_domains);
EXTERNC char *midlib_get_counter_label(MidlibEnergyDomains *enabled_domains, const char *counter);
EXTERNC char *midlib_get_enabled_domain_names(MidlibEnergyDomains *enabled_domains); // NO NEED TO EXPOSE BUT I'LL KEEP IT HERE JUST IN CASE.

// ******** DO WE NEED THESE??? ********
// EXTERNC double midlib_get_result_value(MidlibEnergyDomains *enabled_domains, unsigned long long startValue, unsigned long long stopValue, double runtime, int *cumulative, const char *counter);
// EXTERNC void midlib_get_counter_values(unsigned long long *record);
// EXTERNC void midlib_get_number_of_counters(unsigned int *num);

// undef region
#undef EXTERNC

#endif // SRC_MIDLIB_MIDLIB_H_
