#ifndef COUNTDOWN_MERIC_H_
#define COUNTDOWN_MERIC_H_
#include <stdbool.h>

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

typedef struct MidlibEnergyDomains
{
    enum Domains
    {
        NO_SELECTION = 0, // 00000 No Domains Selected
        A64FX = 1 << 0,   // 00001
        RAPL = 1 << 1,    // 00010
        NVML = 1 << 2,    // 00100
        ROCM = 1 << 3,    // 01000
        HDEEM = 1 << 4    // 10000
    };
    unsigned int value = NO_SELECTION;
} MidlibEnergyDomains;

// Helper macros for bitwise operations
#define MIDLIB_ENERGY_ENABLE_DOMAIN(enabled_domains, domain) ((enabled_domains).value |= (domain))
#define MIDLIB_ENERGY_DISABLE_DOMAIN(enabled_domains, domain) ((enabled_domains).value &= ~(domain))
#define MIDLIB_ENERGY_HAS_DOMAIN(enabled_domains, domain) (((enabled_domains).value & (domain)) != 0)
#define MIDLIB_ENERGY_NO_SELECTION(enabled_domains) ((enabled_domains).value == MidlibEnergyDomains::NO_SELECTION)

EXTERNC void midlib_init(bool is_detailed, MidlibEnergyDomains enabled_domains);
EXTERNC void midlib_read_energy_measurements(unsigned long long int *cpuEnergy, unsigned long long int *dramEnergy, unsigned long long int *L2Energy);
EXTERNC void midlib_getCounterValues(unsigned long long *record);
EXTERNC void midlib_getNumberOfCounters(unsigned int *num);
EXTERNC void midlib_getCounterLabel(int id, char *label);
EXTERNC double midlib_getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, int *cumulative, int counterID);
EXTERNC void midlib_close();

EXTERNC void midlib_read_energy_a64fx(unsigned long long int *cpuEnergy, unsigned long long int *dramEnergy, unsigned long long int *L2Energy);
// no need externc afterwards
#undef EXTERNC
#endif // COUNTDOWN_MERIC_H
