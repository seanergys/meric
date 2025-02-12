# MERICext library directly exposing MERIC internal implementation 

This library wraps a portion of MERIC original codebase, enabling energy consumption measurement across supported power monitoring systems.

Users must enable the desired power monitoring systems using `ExtlibEnergy` struct and `EXTLIB_ENERGY_ENABLE_DOMAIN` macro then call `extlib_init(...)` to initialize the library.

Function `extlib_calc_energy_consumption` returns energy consumed (Joules) in the meantime of two timestamps which must be read using `extlib_read_energy_measurements` (raw data with in internal power monitoring system specific unit). Both functions return `ExtlibEnergyTimeStamp` struct which keeps data for each power domain. 



**Note:** The methods `extlib_read_energy_measurements(...)` and `extlib_calc_energy_consumption(...)` return a heap-allocated `ExtlibEnergyTimeStamp` object. Please use `extlib_free_energy_timestamp(energy_timestamp)` to free the allocated memory appropriately. For methods that return a `char*`, you can use the `free(..)`.

An example program is provided below. For the full interface, refer to `meric/src/extlib/extlib.h`, and for more comprehensive example see `meric/src/test/extlib_test.c`.

```
// extlib_example.c

#include "extlib.h"

ExtlibEnergy energy_domains = {0};
EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, EXTLIB_ENERGY_DOMAIN_A64FX);

extlib_init(energy_domains, true);

struct ExtlibEnergyTimeStamp *ts = extlib_read_energy_measurements(&energy_domains);

// some code of interest

struct ExtlibEnergyTimeStamp *ts2 = extlib_read_energy_measurements(&energy_domains);


struct ExtlibEnergyTimeStamp *result = extlib_calc_energy_consumption(ts, ts2);
extlib_print_energy_timestamp(result);


extlib_free_energy_timestamp(ts);
extlib_free_energy_timestamp(ts2);
extlib_free_energy_timestamp(result);


extlib_close(energy_domains);

```
