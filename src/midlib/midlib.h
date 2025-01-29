#ifndef SRC_MIDLIB_MIDLIB_H_
#define SRC_MIDLIB_MIDLIB_H_

#include <stdbool.h>
#include <stdint.h>  // Include this header for uint64_t

/*
    Return timestemp per call,
    add c++ methods to calculate (subtract) 2 timestemp provided to return the actual value, thus it will be supported for nasted versions.

*/

// return this timestamp upon calling for an energy reading.
struct MidlibEnergyTimeStamp
{
    double id;
    uint64_t* a64fx;  // pointers for an array.    
    size_t a64fx_arr_size;

    uint64_t* rapl;   // pointers for an array.
    size_t rapl_arr_size;

    uint64_t* nvml;   // pointers for an array.
    size_t nvml_arr_size;

    uint64_t* rocm;   // pointers for an array.
    size_t rocm_arr_size;

    uint64_t* hdeem;  // pointers for an array.    
    size_t hdeem_arr_size;
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

    uint64_t* (*energyMeasurementFns[5])(size_t*, double); // Function pointers to energy measurements for enabled domains
    void (*printMeasurementFns[5])(bool);          // Function pointers to printing measurements for enabled domains
    void (*closeFns[5])();                         // Close enabled domains
    char *(*counterLabelFNs[5])(const char *);     // Get counter labels for enabled domains
    // double (*getResultValueFNs[5])(unsigned long long, unsigned long long, double, int *, const char *); // Get counter labels for enabled domains

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

EXTERNC struct MidlibEnergyTimeStamp* midlib_read_energy_measurements(MidlibEnergy *enabled_domains);
EXTERNC void midlib_print_timestamp(struct MidlibEnergyTimeStamp *timestamp, size_t print_limit);
EXTERNC void midlib_free_timestamp(struct MidlibEnergyTimeStamp *timestamp);

EXTERNC void midlib_print_energy_measurements(MidlibEnergy *enabled_domains);
EXTERNC char *midlib_get_counter_label(MidlibEnergy *enabled_domains, const char *counter);
EXTERNC char *midlib_get_enabled_domain_names(MidlibEnergy *enabled_domains); // NO NEED TO EXPOSE BUT I'LL KEEP IT HERE JUST IN CASE.


// EXTERNC double midlib_get_result_value(MidlibEnergy *enabled_domains, unsigned long long startValue, unsigned long long stopValue, double runtime, int *cumulative, const char *counter);
// EXTERNC void midlib_get_counter_values(unsigned long long *record);
// EXTERNC void midlib_get_number_of_counters(unsigned int *num);

// undef region
#undef EXTERNC

#endif // SRC_MIDLIB_MIDLIB_H_
