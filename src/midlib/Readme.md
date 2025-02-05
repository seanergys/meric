# MERIC C Wrapper for Energy Measurement

This shared library wraps a portion of MERIC original codebase, enabling energy consumption measurement for C programs across A64FX, RAPL, ROCm, NVML, and HDEEM energy domains.

Users must enable the desired energy measurement domains using the `MidlibEnergy` struct and the `MIDLIB_ENERGY_ENABLE_DOMAIN` macro then call the `midlib_init(...)` for correct initialisation of the library.

Energy measurements function similarly to timing a code segment in C. Instead of using a `timeval` object, the library returns a `MidlibEnergyTimeStamp` object that includes all available energy domains along with the corresponding collected data. Users must call `midlib_calc_energy_consumption(begin, end)` to calculate the energy consumption between two timestamps.


**Note:** The methods `midlib_read_energy_measurements(...)` and `midlib_calc_energy_consumption(...)` return a heap-allocated `MidlibEnergyTimeStamp` object. Please use `midlib_free_energy_timestamp(energy_timestamp)` to free the allocated memory appropriately. For methods that return a `char*`, you can use the `free(..)`.

An example program is provided below. For the full interface, refer to `meric/src/midlib/midlib.h`, and for a more comprehensive example, check `meric/src/test/midlib_test.c`.

```
// midlib_example.c

#include "midlib.h"

MidlibEnergy energy_domains = {0};
MIDLIB_ENERGY_ENABLE_DOMAIN(energy_domains, MIDLIB_ENERGY_DOMAIN_A64FX);

midlib_init(energy_domains, true);

struct MidlibEnergyTimeStamp *ts = midlib_read_energy_measurements(&energy_domains);

// some code of interest

struct MidlibEnergyTimeStamp *ts = midlib_read_energy_measurements(&energy_domains);


struct MidlibEnergyTimeStamp *result = midlib_calc_energy_consumption(ts, ts2);
midlib_print_energy_timestamp(result);


midlib_free_energy_timestamp(ts);
midlib_free_energy_timestamp(ts2);
midlib_free_energy_timestamp(result);


midlib_close(energy_domains);

```
 


