#include <math.h>
#include <omp.h>
#include "work_c.h"
#include <stdio.h>
#include <stdlib.h>

#include "../src/midlib/midlib.h"

int main(int argc, char **argv)
{
    MidlibEnergy energy_domains = {0};
    MIDLIB_ENERGY_ENABLE_DOMAIN(energy_domains, MIDLIB_ENERGY_DOMAIN_A64FX);
    MIDLIB_ENERGY_ENABLE_DOMAIN(energy_domains, MIDLIB_ENERGY_DOMAIN_NVML);
 
    // MIDLIB_ENERGY_ENABLE_DOMAIN(energy_domains, NVML);
    // printf("enabled domains: %s\n",midlib_get_enabled_domain_names(&energy_domains));

    midlib_init(&energy_domains, true);

    // char* label = midlib_get_counter_label(&energy_domains, "EA_CORE");
    // printf("counter labels are: %s\n",label);
    // free(label);

    // char* enabled_domains = midlib_get_enabled_domain_names(&energy_domains);
    // printf("enabled domains: %s\n",enabled_domains);
    // return 0;

    struct MidlibEnergyTimeStamp * ts = midlib_read_energy_measurements(&energy_domains);
    midlib_print_timestamp(ts,5);
    midlib_free_timestamp(ts);


#pragma omp parallel
    do_work(10.0); // cannot be sleep!! A64FX does not accumulate power samples if the HW is idle

    printf("***** AFTER PARALLEL SESSION ******\n");
    struct MidlibEnergyTimeStamp * ts2 = midlib_read_energy_measurements(&energy_domains);
    midlib_print_timestamp(ts2,5);
    midlib_free_timestamp(ts2);

    // midlib_print_energy_measurements(&energy_domains);
    midlib_close(&energy_domains);
    return 0;
}
