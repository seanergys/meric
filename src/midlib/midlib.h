#ifndef SRC_MIDLIB_MIDLIB_H_
#define SRC_MIDLIB_MIDLIB_H_
 
#include <stdbool.h>

static int countTrailingZeros(unsigned int x);

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
        END = 1 << 5      // 100000
    };
    unsigned int value;

    void (*energyMeasurementFns[5])(bool, double); // Function pointers to energy measurements for enabled domains
    void (*printMeasurementFns[5])(bool);          // Function pointers to printing measurements for enabled domains

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
EXTERNC void midlib_read_energy_measurements(MidlibEnergyDomains *enabled_domains);
EXTERNC void midlib_print_energy_measurements(MidlibEnergyDomains *enabled_domains);

// ******** DO WE NEED THESE??? ********
// EXTERNC void midlib_getCounterValues(unsigned long long *record);
// EXTERNC void midlib_getNumberOfCounters(unsigned int *num);
// EXTERNC void midlib_getCounterLabel(int id, char *label);
// EXTERNC double midlib_getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, int *cumulative, int counterID);
// EXTERNC void midlib_close();

static int countTrailingZeros(unsigned int x)
{
    if (x == 0)
    {
        return 0; // Or handle the zero case as needed
    }

#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
    return __builtin_ctz(x);
#elif defined(_MSC_VER)
    unsigned long index;
    _BitScanForward(&index, x);
    return index;
#else
    // Portable implementation (less efficient)
    int count = 0;
    while ((x & 1) == 0)
    {
        x >>= 1;
        count++;
    }
    return count;
#endif
}

// undef region
#undef EXTERNC

#endif // SRC_MIDLIB_MIDLIB_H_
