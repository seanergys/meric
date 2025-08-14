# MERICext library directly exposing MERIC internal implementation 

This library wraps a portion of MERIC original codebase, enabling energy consumption measurement across supported power monitoring systems.

Users must enable the desired power monitoring systems using `ExtlibEnergy` struct and `EXTLIB_ENERGY_ENABLE_DOMAIN` macro then call `extlib_init(...)` to initialize the library.

Function `extlib_calc_energy_consumption` returns energy consumed (Joules) in the meantime of two timestamps which must be read using `extlib_read_energy_measurements` (raw data with in internal power monitoring system specific unit). Both functions return `ExtlibEnergyTimeStamp` struct which keeps data for each power domain. 

**Note:** The methods `extlib_read_energy_measurements(...)` and `extlib_calc_energy_consumption(...)` return a heap-allocated `ExtlibEnergyTimeStamp` object. Please use `extlib_free_energy_timestamp(energy_timestamp)` to free the allocated memory appropriately. For methods that return a `char*`, you can use the `free(..)`.


## Reserved Memory Manager

This library features an advanced **Reserved Memory Manager** designed to optimize performance for frequent energy measurements by minimizing malloc/free overhead. The system uses a sophisticated paging architecture with:

- **Memory Pooling**: Pre-allocated memory pools organized in pages for efficient timestamp allocation
- **Slot Reuse**: Prioritizes reusing empty slots over creating new allocations to minimize fragmentation  
- **Smart Page Management**: Automatically creates new pages when needed and removes empty pages based on configurable thresholds
- **Shared Resources**: Counter names are shared across pages within the same domain to optimize memory usage

**Note:** The methods `extlib_read_energy_measurements(...)` and `extlib_calc_energy_consumption(...)` retrun the same `ExtlibEnergyTimeStamp` object but the arrays pointing to pre-allocated memory. From the user perspective, everything stays the same, user still needs to use `extlib_free_energy_timestamp(energy_timestamp)` to free.

To enable reserved memory mode, use the `EXTLIB_ENERGY_USE_RESERVED_MEMORY` macro which automatically calculates the required memory size (3x the number of measurements to account for begin/end/result timestamps).

An example program is provided below. For the full interface, refer to `meric/src/extlib/meric_ext.h`, and for more comprehensive example see `meric/src/test/extlib_test.c`.

## Basic Usage

```c
// extlib_example.c

#include <meric_ext.h>

ExtlibEnergy energy_domains = {0};
EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, EXTLIB_ENERGY_DOMAIN_A64FX);

extlib_init(&energy_domains, true);

struct ExtlibEnergyTimeStamp *ts = extlib_read_energy_measurements(&energy_domains);

// some code of interest

struct ExtlibEnergyTimeStamp *ts2 = extlib_read_energy_measurements(&energy_domains);

struct ExtlibEnergyTimeStamp *result = extlib_calc_energy_consumption(ts, ts2);
extlib_print_energy_timestamp(result, -1);

extlib_free_energy_timestamp(ts);
extlib_free_energy_timestamp(ts2);
extlib_free_energy_timestamp(result);

extlib_close(&energy_domains);
```

## Reserved Memory Usage Example

```c
// extlib_reserved_example.c

#include <meric_ext.h>

ExtlibEnergy energy_domains = {0};
EXTLIB_ENERGY_ENABLE_DOMAIN(energy_domains, EXTLIB_ENERGY_DOMAIN_A64FX);

// Enable reserved memory for 10 measurements (automatically allocates 30 slots)
// This is the page size, when more read is done, new pages are created with 30 slots and deallocated automatically.
EXTLIB_ENERGY_USE_RESERVED_MEMORY(energy_domains, 10);

extlib_init(&energy_domains, true);

// All allocations now use pre-allocated memory pools
struct ExtlibEnergyTimeStamp *ts1 = extlib_read_energy_measurements(&energy_domains);
struct ExtlibEnergyTimeStamp *ts2 = extlib_read_energy_measurements(&energy_domains);

struct ExtlibEnergyTimeStamp *result = extlib_calc_energy_consumption(ts1, ts2);

// Cleanup - memory is returned to pool, not freed
extlib_free_energy_timestamp(ts1);
extlib_free_energy_timestamp(ts2);
extlib_free_energy_timestamp(result);

extlib_close(&energy_domains);
```
