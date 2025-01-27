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

    void (*energyMeasurementFns[5])(bool, double); // Function pointers to energy measurements for enabled domains
    void (*printMeasurementFns[5])(bool);          // Function pointers to printing measurements for enabled domains
    void (*closeFns[5])();                         // Close enabled domains

} MidlibEnergyDomains;

// Helper macros for bitwise operations
#define MIDLIB_ENERGY_ENABLE_DOMAIN(enabled_domains, domain) ((enabled_domains).value |= (domain))
#define MIDLIB_ENERGY_DISABLE_DOMAIN(enabled_domains, domain) ((enabled_domains).value &= ~(domain))
#define MIDLIB_ENERGY_HAS_DOMAIN(enabled_domains, domain) (((enabled_domains).value & (domain)) != 0)
#define MIDLIB_ENERGY_NO_SELECTION(enabled_domains) ((enabled_domains).value == MidlibEnergyDomains::NO_SELECTION)

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC void midlib_init(MidlibEnergyDomains *enabled_domains, bool is_detailed);
EXTERNC void midlib_close(MidlibEnergyDomains *enabled_domains); 

EXTERNC void midlib_read_energy_measurements(MidlibEnergyDomains *enabled_domains);
EXTERNC void midlib_print_energy_measurements(MidlibEnergyDomains *enabled_domains);
EXTERNC const char* midlib_printDomainName(unsigned int value);

// ******** DO WE NEED THESE??? ********
// EXTERNC void midlib_getCounterValues(unsigned long long *record);
// EXTERNC void midlib_getCounterLabel(int id, char *label);
// EXTERNC void midlib_getNumberOfCounters(unsigned int *num);
// EXTERNC double midlib_getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, int *cumulative, int counterID);

// undef region
#undef EXTERNC

#endif // SRC_MIDLIB_MIDLIB_H_
