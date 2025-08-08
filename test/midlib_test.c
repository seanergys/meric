#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include <meric_ext.h>
#include "work_c.h"

#define ITER 4

struct ExtlibEnergyTimeStamp *buffer[12];
int main(int argc, char **argv)
{
    /*
        -------------- STEP(1) --------------
        Create ExtlibEnergy object and set energy domains you want to measure
    */
    ExtlibEnergy energy_domains = {0};
    EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, 1);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, EXTLIB_ENERGY_DOMAIN_A64FX);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, EXTLIB_ENERGY_DOMAIN_RAPL);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, EXTLIB_ENERGY_DOMAIN_NVML);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, EXTLIB_ENERGY_DOMAIN_HDEEM);
    EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, EXTLIB_ENERGY_DOMAIN_ROCM);
    // EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, EXTLIB_ENERGY_DOMAIN_HWMON);

    // EXTLIB_ENERGY_DISABLE_DOMAIN(energy_domains, EXTLIB_ENERGY_DOMAIN_NVML);

    /*
        -------------- STEP(2) --------------
        You can checked domains that's enabled
    */
    // char *enabled_domains = extlib_get_enabled_domain_names(&energy_domains);
    // printf("enabled domains: %s\n", enabled_domains);
    // free(enabled_domains);

    /*
        -------------- STEP(3) --------------
        Init extlib runtime after you set the energy domains,
     */
    extlib_init(&energy_domains, true);

    for (size_t i = 0; i < ITER; i++)
    {
        printf("\titer:%d\n", i);
        /*
            -------------- STEP(4) --------------
            Get an EnergyTimeStamp for reference before to begin measuring energy of a code.
         */
        struct ExtlibEnergyTimeStamp *ts = extlib_read_energy_measurements(&energy_domains);
        extlib_print_energy_timestamp(ts, 5);
        // exit(0);
        // extlib_free_energy_timestamp(ts);
        // extlib_close(&energy_domains);
        // exit(0);

        //   BEGIN JOB: REGION THAT WE WANNA MEASURE ITS ENERGY CONSUMPTION

        // #pragma omp parallel
        do_work(5.0); // cannot be sleep!! A64FX does not accumulate power samples if the HW is idle

        // END JOB

        /*
            -------------- STEP(5) --------------
            Get Another Energy reading to measure the span.
         */
        struct ExtlibEnergyTimeStamp *ts2 = extlib_read_energy_measurements(&energy_domains);
        extlib_print_energy_timestamp(ts2, 5);
        // extlib_free_energy_timestamp(ts2);
        // extlib_close(&energy_domains);
        // exit(0);

        /*
            -------------- STEP(6) --------------
            Calculate the consumed energy for enabled domains.
         */

        struct ExtlibEnergyTimeStamp *result = extlib_calc_energy_consumption(ts, ts2);
        extlib_print_energy_timestamp(result, 5);

        printf("\t\t*** PAGES %d %d %d\n", ts->reserved_memory_page, ts2->reserved_memory_page, result->reserved_memory_page);

        /*
            -------------- STEP(7) --------------
             Do NOT forget to free the timestamps
         */
        // if (i == 1)
        // {
        //     printf("Freeing page %d for iter %d\n", 0, i);
        //     printf("\t\t*** PAGES %d %d %d\n", buffer[0]->reserved_memory_page, buffer[1]->reserved_memory_page, buffer[2]->reserved_memory_page);
        //     // printf("\t\t*** PAGES %d %d %d\n", buffer[3]->reserved_memory_page, buffer[4]->reserved_memory_page, buffer[5]->reserved_memory_page);

        // extlib_free_energy_timestamp(buffer[0]);
        // extlib_free_energy_timestamp(buffer[1]);
        // extlib_free_energy_timestamp(buffer[2]);

        // extlib_free_energy_timestamp(ts);
        // extlib_free_energy_timestamp(ts2);
        // extlib_free_energy_timestamp(result);
        // }
        // else if (i == 0) // add iter 0 and 1
        // {
        printf("\tAdding to buffer for iter %d\n", i);
        buffer[i * 3] = ts;
        buffer[i * 3 + 1] = ts2;
        buffer[i * 3 + 2] = result;
        // }
    }

    printf("*** DONE MEASURING ***\n");
    for (int i = 0; i < ITER; i++)
    // for (int i = ITER - 1; i >= 0; i--)
    {
        extlib_free_energy_timestamp(buffer[i * 3]);
        extlib_free_energy_timestamp(buffer[i * 3 + 1]);
        extlib_free_energy_timestamp(buffer[i * 3 + 2]);
    }

    printf("*** DONE FREEING ***\n");
    extlib_shrink_to_fit();

    printf("********************* READ AGAIN *********************!!\n");
    for (size_t i = 0; i < 2; i++)
    {
        printf("\titer:%d\n", i);
        struct ExtlibEnergyTimeStamp *ts = extlib_read_energy_measurements(&energy_domains);
        extlib_print_energy_timestamp(ts, 5);

        do_work(5.0); // cannot be sleep!! A64FX does not accumulate power samples if the HW is idle

        struct ExtlibEnergyTimeStamp *ts2 = extlib_read_energy_measurements(&energy_domains);
        extlib_print_energy_timestamp(ts2, 5);

        struct ExtlibEnergyTimeStamp *result = extlib_calc_energy_consumption(ts, ts2);
        extlib_print_energy_timestamp(result, 5);

        printf("\t\t*** PAGES %d %d %d\n", ts->reserved_memory_page, ts2->reserved_memory_page, result->reserved_memory_page);

        extlib_free_energy_timestamp(ts);
        extlib_free_energy_timestamp(ts2);
        extlib_free_energy_timestamp(result);
    }
    /*
        -------------- STEP(8) --------------
        Do NOT forget close extlib for selected energy domains object.
     */
    extlib_close(&energy_domains);
    return 0;
}
