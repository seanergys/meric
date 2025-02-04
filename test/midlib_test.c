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
        Create MidlibEnergy object and set energy domains you want to measure
    */
    MidlibEnergy energy_domains = {0};
    MIDLIB_ENERGY_ENABLE_DOMAIN(energy_domains, MIDLIB_ENERGY_DOMAIN_A64FX);
    // MIDLIB_ENERGY_ENABLE_DOMAIN(energy_domains, MIDLIB_ENERGY_DOMAIN_RAPL);
    // MIDLIB_ENERGY_ENABLE_DOMAIN(energy_domains, MIDLIB_ENERGY_DOMAIN_NVML);
    // MIDLIB_ENERGY_ENABLE_DOMAIN(energy_domains, MIDLIB_ENERGY_DOMAIN_HDEEM);
    // MIDLIB_ENERGY_ENABLE_DOMAIN(energy_domains, MIDLIB_ENERGY_DOMAIN_ROCM);

    // MIDLIB_ENERGY_DISABLE_DOMAIN(energy_domains, MIDLIB_ENERGY_DOMAIN_NVML);

    /*
        -------------- STEP(2) --------------
        You can checked domains that's enabled
    */
    // char *enabled_domains = midlib_get_enabled_domain_names(&energy_domains);
    // printf("enabled domains: %s\n", enabled_domains);
    // free(enabled_domains);

    /*
        -------------- STEP(3) --------------
        Init midlib runtime after you set the energy domains,
     */
    midlib_init(&energy_domains, true);

    /*
        -------------- STEP(4) --------------
        Get an EnergyTimeStamp for reference before to begin measuring energy of a code.
     */
    struct MidlibEnergyTimeStamp *ts = midlib_read_energy_measurements(&energy_domains);
    // midlib_print_energy_timestamp(ts, 5);

    
    //   BEGIN JOB: REGION THAT WE WANNA MEASURE ITS ENERGY CONSUMPTION
     
#pragma omp parallel
    do_work(10.0); // cannot be sleep!! A64FX does not accumulate power samples if the HW is idle

    // END JOB
     

    /*
        -------------- STEP(5) --------------
        Get Another Energy reading to measure the span.
     */
    struct MidlibEnergyTimeStamp *ts2 = midlib_read_energy_measurements(&energy_domains);
    // midlib_print_energy_timestamp(ts2, 5);

    /*
        -------------- STEP(6) --------------
        Calculate the consumed energy for enabled domains.
     */

    struct MidlibEnergyTimeStamp *result = midlib_calc_energy_consumption(ts, ts2);
    midlib_print_energy_timestamp(result, 5);

    /*
        -------------- STEP(7) --------------
         Do NOT forget to free the timestamps
     */
    midlib_free_energy_timestamp(ts);
    midlib_free_energy_timestamp(ts2);
    midlib_free_energy_timestamp(result);

    /*
        -------------- STEP(8) --------------
        Do NOT forget close midlib for selected energy domains object.
     */
    midlib_close(&energy_domains);
    return 0;
}
