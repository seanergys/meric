#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "work_c.h"
#include <meric_ext.h>

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
	EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, EXTLIB_ENERGY_DOMAIN_HWMON);

	extlib_init(&energy_domains, true);

	struct ExtlibEnergyTimeStamp *ts = extlib_read_energy_measurements(&energy_domains);
	extlib_print_energy_timestamp(ts, 5);

	// #pragma omp parallel
	do_work(5.0);

	struct ExtlibEnergyTimeStamp *ts2 = extlib_read_energy_measurements(&energy_domains);
	extlib_print_energy_timestamp(ts2, 5);

	struct ExtlibEnergyTimeStamp *result = extlib_calc_energy_consumption(ts, ts2);
	extlib_print_energy_timestamp(result, 5);

	extlib_close(&energy_domains);
	return 0;
}
