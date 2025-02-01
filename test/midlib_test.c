#include <math.h>
#include <omp.h>
#include "work_c.h"
#include <stdio.h>
#include <stdlib.h>

#include "../src/midlib/midlib.h"

int main(int argc, char **argv)
{
    /*
                    -------------- STEP(1) --------------
        * Create MidlibEnergy object and set energy domains you want to measure
    */
    MidlibEnergy energy_domains = {0};
    MIDLIB_ENERGY_ENABLE_DOMAIN(energy_domains, MIDLIB_ENERGY_DOMAIN_NVML);
    MIDLIB_ENERGY_ENABLE_DOMAIN(energy_domains, MIDLIB_ENERGY_DOMAIN_RAPL);
    //MIDLIB_ENERGY_DISABLE_DOMAIN(energy_domains, MIDLIB_ENERGY_DOMAIN_NVML);

    /*
                    -------------- STEP(2) --------------
        * You can checked domains that's enabled at tne end
    */
    char *enabled_domains = midlib_get_enabled_domain_names(&energy_domains);
    printf("enabled domains: %s\n", enabled_domains);
    free(enabled_domains);

    /*
                    -------------- STEP(3) --------------
        * Init midlib runtime,
        * it initialises energy domain settings & maps function pointers
    */
    midlib_init(&energy_domains, true);

    /*
                    -------------- STEP(4) --------------
        * call midlib_read_energy_measurements with energy_domains object that you enabled domains.
        * it will return an EnergyTimeStamp object to you
        * you can print the object with midlib_print_energy_timestamp(ts, # of values to be displayed)
        * Each timeStamp object will hold the energy counter readings for enabled (and existsted) domain.
        * In this case A64Fx domain exists in the test machine, you can see the printed output below
        *
        * -------------------------------- (id value is the timestamp when the object is created during energy measurement.)
        *                                |
        *                                v
        * ------- MidlibEnergyTimeStamp (id: 1738250797.6328) -------
        * | a64fx_counter_name (size: 56): [EA_CORE_00, EA_CORE_01, EA_CORE_02, EA_CORE_03, EA_CORE_04, ...]
        * | a64fx_counter_value (size: 56): [251544, 260996, 261868, 280186, 261523, ...]
        * | a64fx_energy_per_counter (size: 56): NULL or empty array
        * | a64fx_total_energy: 0.000000
        * |
        * | rapl_counter_name (size: 0): NULL or empty array
        * | rapl_counter_value (size: 0): NULL or empty array
        * | rapl_energy_per_counter (size: 0): NULL or empty array
        * | rapl_total_energy: 0.000000
        * |
        * | nvml_counter_name (size: 0): NULL or empty array
        * | nvml_counter_value (size: 0): NULL or empty array
        * | nvml_energy_per_counter (size: 0): NULL or empty array
        * | nvml_total_energy: 0.000000
        * |
        * | rocm_counter_name (size: 0): NULL or empty array
        * | rocm_counter_value (size: 0): NULL or empty array
        * | rocm_energy_per_counter (size: 0): NULL or empty array
        * | rocm_total_energy: 0.000000
        * |
        * | hdeem_counter_name (size: 0): NULL or empty array
        * | hdeem_counter_value (size: 0): NULL or empty array
        * | hdeem_energy_per_counter (size: 0): NULL or empty array
        * | hdeem_total_energy: 0.000000
        * |
        * | Total_energy consumption: 0.000000 [J]
        * -----------------------------------

    */
    struct MidlibEnergyTimeStamp *ts = midlib_read_energy_measurements(&energy_domains);
    midlib_print_energy_timestamp(ts, 5);

    /*
     *   BEGIN JOB
     */

#pragma omp parallel
    do_work(10.0); // cannot be sleep!! A64FX does not accumulate power samples if the HW is idle

    /*
     *   END JOB
     */

    /*
                        -------------- STEP(5) --------------
        * Get Another Energy reading
     */
    struct MidlibEnergyTimeStamp *ts2 = midlib_read_energy_measurements(&energy_domains);
    midlib_print_energy_timestamp(ts2, 5);



    struct MidlibEnergyTimeStamp *result = midlib_calc_energy_consumption(ts, ts2);
    midlib_print_energy_timestamp(result, 5);
    
    /*
     *  Call midlib_calc_energy_consumption(..) to calculate energy consumption
     *  It returns another timeStamp object only consisting the energy consumption values
     *  The output of resulting timestamp is given below,
     *      -> you have the total energy consumption per domain ie. a64fx_total_energy, rapl_total_energy...
     *      -> you also have the total energy consumption; Total_energy consumption: 1108.238159 [J], which combines all per domain energy.
     *
     *   ------- MidlibEnergyTimeStamp (id: 1738250807.6350) -------
     *   | a64fx_counter_name (size: 56): NULL or empty array
     *   | a64fx_counter_value (size: 56): NULL or empty array
     *   | a64fx_energy_per_counter (size: 56): [20.003511, 22.578263, 20.002508, 20.511014, 20.002596, ...]
     *   | a64fx_total_energy: 1108.238159
     *   |
     *   | rapl_counter_name (size: 0): NULL or empty array
     *   | rapl_counter_value (size: 0): NULL or empty array
     *   | rapl_energy_per_counter (size: 0): NULL or empty array
     *   | rapl_total_energy: 0.000000
     *   |
     *   | nvml_counter_name (size: 0): NULL or empty array
     *   | nvml_counter_value (size: 0): NULL or empty array
     *   | nvml_energy_per_counter (size: 0): NULL or empty array
     *   | nvml_total_energy: 0.000000
     *   |
     *   | rocm_counter_name (size: 0): NULL or empty array
     *   | rocm_counter_value (size: 0): NULL or empty array
     *   | rocm_energy_per_counter (size: 0): NULL or empty array
     *   | rocm_total_energy: 0.000000
     *   |
     *   | hdeem_counter_name (size: 0): NULL or empty array
     *   | hdeem_counter_value (size: 0): NULL or empty array
     *   | hdeem_energy_per_counter (size: 0): NULL or empty array
     *   | hdeem_total_energy: 0.000000
     *   |
     *   | Total_energy consumption: 1108.238159 [J]
     *   -----------------------------------
     */

    /*
                        -------------- STEP(6) --------------
        *   Do NOT forget to free the timestamps
     */
    midlib_free_energy_timestamp(ts);
    midlib_free_energy_timestamp(ts2);
    midlib_free_energy_timestamp(result);

    /*
                        -------------- STEP(7) --------------
        *   Do NOT forget close midlib for selected energy domains object.
     */
    midlib_close(&energy_domains);
    return 0;
}
