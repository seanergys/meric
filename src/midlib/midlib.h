#ifndef SRC_MIDLIB_MIDLIB_H_
#define SRC_MIDLIB_MIDLIB_H_

#include <stdbool.h>
#include <stdint.h> // Include this header for uint64_t

/*
    Return timestemp per call,
    add c++ methods to calculate (subtract) 2 timestemp provided to return the actual value, thus it will be supported for nasted versions.

*/

#define MIDLIB_NUM_DOMAINS 5
// return this timestamp upon calling for an energy reading.
struct MidlibEnergyTimeStamp
{
    double id;

    // TODO: convert these to a structure of an array, get size from MIDLIB_NUM_DOMAINS, use countTrailingZeros(domain) to get the corresponding index.
    // TODO: this will simplify the free_timestamp drastically, also simplify midlib_calc_energy_consumption.. 
    
    size_t a64fx_arr_size;            // same for each array below.
    char **a64fx_counter_name;        // counter names
    uint64_t *a64fx_counter_value;    // counter values (need to be converted to joules)
    double *a64fx_energy_per_counter; // consumed energy per counter
    double a64fx_energy_total;        // total energy

    size_t rapl_arr_size;            // same for each array below.
    char **rapl_counter_name;        // counter names
    uint64_t *rapl_counter_value;    // counter values (need to be converted to joules)
    double *rapl_energy_per_counter; // consumed energy per counter
    double rapl_energy_total;        // total energy

    size_t nvml_arr_size;            // same for each array below.
    char **nvml_counter_name;        // counter names
    uint64_t *nvml_counter_value;    // counter values (need to be converted to joules)
    double *nvml_energy_per_counter; // consumed energy per counter
    double nvml_energy_total;        // total energy

    size_t rocm_arr_size;            // same for each array below.
    char **rocm_counter_name;        // counter names
    uint64_t *rocm_counter_value;    // counter values (need to be converted to joules)
    double *rocm_energy_per_counter; // consumed energy per counter
    double rocm_energy_total;        // total energy

    size_t hdeem_arr_size;            // same for each array below.
    char **hdeem_counter_name;        // counter names
    uint64_t *hdeem_counter_value;    // counter values (need to be converted to joules)
    double *hdeem_energy_per_counter; // consumed energy per counter
    double hdeem_energy_total;        // total energy

    double energy_total;
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
