#ifndef SRC_MIDLIB_MIDLIB_H_
#define SRC_MIDLIB_MIDLIB_H_

#include <limits>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <omp.h>
#include <stdbool.h>
#include "../wrapper/environmentwrapper.h"
#include "../wrapper/a64fxwrapper.h"
#include "../wrapper/raplwrapper.h"
#include "../wrapper/nvmlwrapper.h"
#include "../wrapper/rocmwrapper.h"
#include "../wrapper/hdeemwrapper.h"
#include "../basis/utils.h"
#include "../basis/cpu_optimisations.h"

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

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
    unsigned int value = NO_SELECTION;

    typedef void (*EnergyMeasurementFn)(bool, double);
    typedef void (*PrintMeasurementFn)(bool);
    EnergyMeasurementFn energyMeasurementFns[5]; // Corresponds to 5 domains
    PrintMeasurementFn printMeasurementFns[5];   // Corresponds to 5 domains

} MidlibEnergyDomains;

// Helper macros for bitwise operations
#define MIDLIB_ENERGY_ENABLE_DOMAIN(enabled_domains, domain) ((enabled_domains).value |= (domain))
#define MIDLIB_ENERGY_DISABLE_DOMAIN(enabled_domains, domain) ((enabled_domains).value &= ~(domain))
#define MIDLIB_ENERGY_HAS_DOMAIN(enabled_domains, domain) (((enabled_domains).value & (domain)) != 0)
#define MIDLIB_ENERGY_NO_SELECTION(enabled_domains) ((enabled_domains).value == MidlibEnergyDomains::NO_SELECTION)

EXTERNC void midlib_init(MidlibEnergyDomains *enabled_domains,bool is_detailed);
EXTERNC void midlib_read_energy_measurements(MidlibEnergyDomains *enabled_domains);
EXTERNC void midlib_print_energy_measurements(MidlibEnergyDomains *enabled_domains);
EXTERNC void midlib_getCounterValues(unsigned long long *record);
EXTERNC void midlib_getNumberOfCounters(unsigned int *num);
EXTERNC void midlib_getCounterLabel(int id, char *label);
EXTERNC double midlib_getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, int *cumulative, int counterID);
EXTERNC void midlib_close();

// no need externc afterwards
#undef EXTERNC
#endif // COUNTDOWN_MERIC_H
