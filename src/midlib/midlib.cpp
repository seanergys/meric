#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <omp.h>
#include <stdbool.h>

#include "midlib.h"
#include "../basis/utils.h"
#include "../basis/cpu_optimisations.h"
#include "../wrapper/environmentwrapper.h"
#include "../wrapper/a64fxwrapper.h"
#include "../wrapper/raplwrapper.h"
#include "../wrapper/nvmlwrapper.h"
#include "../wrapper/rocmwrapper.h"
#include "../wrapper/hdeemwrapper.h"
#include "../basis/cpu_optimisations.h"

/*
    --------- Compilation Unit Only Method Declerations ---------

    These methods are not present in the header file &
    may or maynot be called indirectly by the midlib_* methods

*/
struct MidlibEnergyTimeStamp *initEnergyTimeStamp();
static const char *printDomainName(unsigned int value);
static void printArray(const char *name, uint64_t *arr, size_t size, size_t print_limit);
static void printArray(const char *name, char **arr, size_t size, size_t print_limit);
static void printArray(const char *name, double *arr, size_t size, size_t print_limit);

template <typename Tmode> static uint64_t *measureEnergy(size_t *size, double timeNow = 0.0);
template <typename Tmode> static void close();

/* --------------------------------------------------------------- */


template <typename Tmode>
static void measureEnergy(size_t *size, uint64_t **values, char ***labels)
{
    std::map<std::string, unsigned long long int> energyNow;
    Tmode::getCounterValues(energyNow);

    *size = energyNow.size();
    *values = (uint64_t *)malloc(*size * sizeof(uint64_t));
    *labels = (char **)malloc(*size * sizeof(char *));

    if (!(*values) || !(*labels))
    {
        std::cerr << "Memory allocation failed!\n";
        return;
    }

    int i = 0;
    for (const auto &entry : energyNow)
    {
        (*values)[i] = entry.second; // Store energy value

        // Allocate and copy the string key
        (*labels)[i] = (char *)malloc(entry.first.size() + 1);
        if ((*labels)[i])
        {
            strcpy((*labels)[i], entry.first.c_str());
        }
        i++;
    }
}

template void measureEnergy<meric::A64FX>(size_t *, uint64_t **, char ***);
template void measureEnergy<meric::RAPL>(size_t *, uint64_t **, char ***);
template void measureEnergy<meric::NVML>(size_t *, uint64_t **, char ***);
// template void measureEnergy<meric::ROCM>(size_t*, uint64_t**, char***);
// template void measureEnergy<meric::HDEEM>(size_t*, uint64_t**, char***);

template <typename Tmode>
static void close()
{
    Tmode::close();
}
template void close<meric::A64FX>();
template void close<meric::RAPL>();
template void close<meric::NVML>();
// template void close<meric::ROCM>();
// template void close<meric::HDEEM>();

void midlib_init(MidlibEnergy *enabled_domains, bool is_detailed)
{
    if (OPT_UNLIKELY(MIDLIB_ENERGY_NO_SELECTION(*enabled_domains)))
    {
        perror("Error: No Domain Selected!\n");
        return;
    }

    meric::Environment::initHWSizes();

    for (unsigned int domain = 1; domain < MidlibEnergy::MIDLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        int index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = NULL;
        enabled_domains->closeFns[index] = NULL;
    }

    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_A64FX)))
    {
        meric::A64FX::init(true);
        int index = meric::mutils::countTrailingZeros(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_A64FX); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::A64FX>;
        enabled_domains->closeFns[index] = close<meric::A64FX>;
        printf("Domain %s is init [OK]\n", printDomainName(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_A64FX));
    }
    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_RAPL)))
    {
        meric::RAPL::init(true);
        int index = meric::mutils::countTrailingZeros(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_RAPL); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::RAPL>;
        enabled_domains->closeFns[index] = close<meric::RAPL>;
        printf("Domain %s is init [OK]\n", printDomainName(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_RAPL));
    }
    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_NVML)))
    {
        meric::NVML::init(true);
        int index = meric::mutils::countTrailingZeros(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_NVML); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::NVML>;
        enabled_domains->closeFns[index] = close<meric::NVML>;
        printf("Domain %s is init [OK]\n", printDomainName(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_NVML));
    }
    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_ROCM)))
    {
        perror("ROCM Has not yet been implemented!\n");
        // meric::ROCM::init(true);
        // int index = meric::mutils::countTrailingZeros(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_ROCM); // Get index of the enabled domain
        // enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::ROCM>;
        // enabled_domains->closeFns[index] = close<meric::ROCM>;
        // printf("Domain %s is init [OK]\n", printDomainName(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_ROCM));
    }
    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_HDEEM)))
    {
        perror("HDEEM Has not yet been implemented!\n");
        // meric::HDEEM::init(true);
        // int index = meric::mutils::countTrailingZeros(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_HDEEM); // Get index of the enabled domain
        // enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::HDEEM>;
        // enabled_domains->closeFns[index] = close<meric::HDEEM>;
        // printf("Domain %s is init [OK]\n", printDomainName(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_HDEEM));
    }
}

static const char *printDomainName(unsigned int value)
{
    switch (value)
    {
    case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_A64FX:
        return "A64FX";
        break;
    case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_RAPL:
        return "RAPL";
        break;
    case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_NVML:
        return "NVML";
        break;
    case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_ROCM:
        return "ROCM";
        break;
    case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_HDEEM:
        return "HDEEM";
        break;
    case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_END:
        return "END";
        break;
    case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_NO_SELECTION:
        return "NO_SELECTION";
        break;
    default:
        return "Unknown Domain";
        break;
    }
}


void midlib_close(MidlibEnergy *enabled_domains)
{
    for (unsigned int domain = 1; domain < MidlibEnergy::MIDLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain)) // Check if the domain is enabled
        {
            int index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
            if (enabled_domains->closeFns[index])
            {
                enabled_domains->closeFns[index]();
                printf("Domain %s is closed [OK]\n", printDomainName(domain));
            }
        }
    }
}

MidlibEnergyTimeStamp *midlib_read_energy_measurements(MidlibEnergy *enabled_domains)
{
    struct MidlibEnergyTimeStamp *timestamp = initEnergyTimeStamp();

    for (unsigned int domain = 1; domain < MidlibEnergy::MIDLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain)) // Check if the domain is enabled
        {
            int index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
            if (enabled_domains->energyMeasurementFns[index])
            {
                size_t arr_size;
                uint64_t *counter_value;
                char **counter_label;
                enabled_domains->energyMeasurementFns[index](&arr_size, &counter_value, &counter_label);

                switch (domain)
                {
                case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_A64FX:
                    timestamp->a64fx_counter_value = counter_value;
                    timestamp->a64fx_counter_name = counter_label;
                    timestamp->a64fx_arr_size = arr_size;
                    break;
                case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_RAPL:
                    timestamp->rapl_counter_value = counter_value;
                    timestamp->rapl_counter_name = counter_label;
                    timestamp->rapl_arr_size = arr_size;
                    break;
                case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_NVML:
                    timestamp->nvml_counter_value = counter_value;
                    timestamp->nvml_counter_name = counter_label;
                    timestamp->nvml_arr_size = arr_size;
                    break;
                case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_ROCM:
                    timestamp->rocm_counter_value = counter_value;
                    timestamp->rocm_counter_name = counter_label;
                    timestamp->rocm_arr_size = arr_size;
                    break;
                case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_HDEEM:
                    timestamp->hdeem_counter_value = counter_value;
                    timestamp->hdeem_counter_name = counter_label;
                    timestamp->hdeem_arr_size = arr_size;
                    break;
                default:
                    break;
                }
            }

            printf("Energy is read from Domain %s [OK]\n", printDomainName(domain));
        }
    }
    return timestamp;
}

// Helper function to print an array with a limit on the number of elements printed
static void printArray(const char *name, uint64_t *arr, size_t size, size_t print_limit)
{
    printf("| %-15s (size: %zu): ", name, size);

    if (arr == NULL || size == 0)
    {
        printf("NULL or empty array\n");
        return;
    }

    printf("[");

    print_limit = print_limit == -1 ? size : print_limit;
    for (size_t i = 0; i < size && i < print_limit; ++i)
    {
        printf("%lu", arr[i]);
        if (i < size - 1 && i < print_limit - 1)
        {
            printf(", ");
        }
    }

    if (size > print_limit)
    {
        printf(", ..."); // Indicate that there are more elements
    }

    printf("]\n");
}

// Helper function to print an array with a limit on the number of elements printed
static void printArray(const char *name, double *arr, size_t size, size_t print_limit)
{
    printf("| %-15s (size: %zu): ", name, size);

    if (arr == NULL || size == 0)
    {
        printf("NULL or empty array\n");
        return;
    }

    printf("[");

    print_limit = print_limit == -1 ? size : print_limit;
    for (size_t i = 0; i < size && i < print_limit; ++i)
    {
        printf("%f", arr[i]);
        if (i < size - 1 && i < print_limit - 1)
        {
            printf(", ");
        }
    }

    if (size > print_limit)
    {
        printf(", ..."); // Indicate that there are more elements
    }

    printf("]\n");
}

// Helper function to print an array with a limit on the number of elements printed
static void printArray(const char *name, char **arr, size_t size, size_t print_limit)
{
    printf("| %-15s (size: %zu): ", name, size);

    if (arr == NULL || size == 0)
    {
        printf("NULL or empty array\n");
        return;
    }

    printf("[");

    print_limit = print_limit == -1 ? size : print_limit;
    for (size_t i = 0; i < size && i < print_limit; ++i)
    {
        printf("%s", arr[i]);
        if (i < size - 1 && i < print_limit - 1)
        {
            printf(", ");
        }
    }

    if (size > print_limit)
    {
        printf(", ..."); // Indicate that there are more elements
    }

    printf("]\n");
}


struct MidlibEnergyTimeStamp *initEnergyTimeStamp()
{
    struct MidlibEnergyTimeStamp *energy_timestamp = (struct MidlibEnergyTimeStamp *)malloc(sizeof(struct MidlibEnergyTimeStamp));
    memset(energy_timestamp, 0, sizeof(struct MidlibEnergyTimeStamp)); // Correct: clears struct fields

    struct timespec tt;
    clock_gettime(CLOCK_REALTIME, &tt);
    energy_timestamp->id = meric::mutils::timeToDouble(tt);
    return energy_timestamp;
}


void midlib_print_energy_timestamp(struct MidlibEnergyTimeStamp *timestamp, size_t print_limit)
{
    if (OPT_UNLIKELY(timestamp == NULL))
    {
        perror("Error: NULL timestamp\n");
        return;
    }
    // Print pointer addresses and array sizes
    printf("------- MidlibEnergyTimeStamp (id: %.4f) -------\n", timestamp->id);

    printArray("a64fx_counter_name", timestamp->a64fx_counter_name, timestamp->a64fx_arr_size, print_limit);
    printArray("a64fx_counter_value", timestamp->a64fx_counter_value, timestamp->a64fx_arr_size, print_limit);
    printArray("a64fx_energy_per_counter", timestamp->a64fx_energy_per_counter, timestamp->a64fx_arr_size, print_limit);
    printf("| a64fx_total_energy: %lf\n", timestamp->energy_total);

    printf("|\n");
    printArray("rapl_counter_name", timestamp->rapl_counter_name, timestamp->rapl_arr_size, print_limit);
    printArray("rapl_counter_value", timestamp->rapl_counter_value, timestamp->rapl_arr_size, print_limit);
    printArray("rapl_energy_per_counter", timestamp->rapl_energy_per_counter, timestamp->rapl_arr_size, print_limit);
    printf("| rapl_total_energy: %lf\n", timestamp->rapl_energy_total);

    printf("|\n");
    printArray("nvml_counter_name", timestamp->nvml_counter_name, timestamp->nvml_arr_size, print_limit);
    printArray("nvml_counter_value", timestamp->nvml_counter_value, timestamp->nvml_arr_size, print_limit);
    printArray("nvml_energy_per_counter", timestamp->nvml_energy_per_counter, timestamp->nvml_arr_size, print_limit);
    printf("| nvml_total_energy: %lf\n", timestamp->nvml_energy_total);

    printf("|\n");
    printArray("rocm_counter_name", timestamp->rocm_counter_name, timestamp->rocm_arr_size, print_limit);
    printArray("rocm_counter_value", timestamp->rocm_counter_value, timestamp->rocm_arr_size, print_limit);
    printArray("rocm_energy_per_counter", timestamp->rocm_energy_per_counter, timestamp->rocm_arr_size, print_limit);
    printf("| rocm_total_energy: %lf\n", timestamp->rocm_energy_total);

    printf("|\n");
    printArray("hdeem_counter_name", timestamp->hdeem_counter_name, timestamp->hdeem_arr_size, print_limit);
    printArray("hdeem_counter_value", timestamp->hdeem_counter_value, timestamp->hdeem_arr_size, print_limit);
    printArray("hdeem_energy_per_counter", timestamp->hdeem_energy_per_counter, timestamp->hdeem_arr_size, print_limit);
    printf("| hdeem_total_energy: %lf\n", timestamp->hdeem_energy_total);

    printf("|\n");
    printf("| Total_energy consumption: %lf [J]\n", timestamp->energy_total);

    printf("-----------------------------------\n");
}
 
void midlib_free_energy_timestamp(struct MidlibEnergyTimeStamp *timestamp)
{
    if (OPT_UNLIKELY(timestamp == NULL))
        return;

    // Free a64fx arrays
    if (OPT_LIKELY(timestamp->a64fx_counter_value != NULL))
        free(timestamp->a64fx_counter_value);

    if (OPT_LIKELY(timestamp->a64fx_energy_per_counter != NULL))
        free(timestamp->a64fx_energy_per_counter);

    if (OPT_LIKELY(timestamp->a64fx_counter_name != NULL))
    {
        for (size_t i = 0; i < timestamp->a64fx_arr_size; i++)
            free(timestamp->a64fx_counter_name[i]);
        free(timestamp->a64fx_counter_name);
    }

    // Free rapl arrays
    if (OPT_LIKELY(timestamp->rapl_counter_value != NULL))
        free(timestamp->rapl_counter_value);

    if (OPT_LIKELY(timestamp->rapl_energy_per_counter != NULL))
        free(timestamp->rapl_energy_per_counter);

    if (OPT_LIKELY(timestamp->rapl_counter_name != NULL))
    {
        for (size_t i = 0; i < timestamp->rapl_arr_size; i++)
            free(timestamp->rapl_counter_name[i]);
        free(timestamp->rapl_counter_name);
    }

    // Free nvml arrays
    if (OPT_LIKELY(timestamp->nvml_counter_value != NULL))
        free(timestamp->nvml_counter_value);

    if (OPT_LIKELY(timestamp->nvml_energy_per_counter != NULL))
        free(timestamp->nvml_energy_per_counter);

    if (OPT_LIKELY(timestamp->nvml_counter_name != NULL))
    {
        for (size_t i = 0; i < timestamp->nvml_arr_size; i++)
            free(timestamp->nvml_counter_name[i]);
        free(timestamp->nvml_counter_name);
    }

    // Free rocm arrays
    if (OPT_LIKELY(timestamp->rocm_counter_value != NULL))
        free(timestamp->rocm_counter_value);

    if (OPT_LIKELY(timestamp->rocm_energy_per_counter != NULL))
        free(timestamp->rocm_energy_per_counter);

    if (OPT_LIKELY(timestamp->rocm_counter_name != NULL))
    {
        for (size_t i = 0; i < timestamp->rocm_arr_size; i++)
            free(timestamp->rocm_counter_name[i]);
        free(timestamp->rocm_counter_name);
    }

    // Free hdeem arrays
    if (OPT_LIKELY(timestamp->hdeem_counter_value != NULL))
        free(timestamp->hdeem_counter_value);

    if (OPT_LIKELY(timestamp->hdeem_energy_per_counter != NULL))
        free(timestamp->hdeem_energy_per_counter);

    if (OPT_LIKELY(timestamp->hdeem_counter_name != NULL))
    {
        for (size_t i = 0; i < timestamp->hdeem_arr_size; i++)
            free(timestamp->hdeem_counter_name[i]);
        free(timestamp->hdeem_counter_name);
    }

    free(timestamp);
}

struct MidlibEnergyTimeStamp *midlib_calc_energy_consumption(MidlibEnergyTimeStamp *begin, MidlibEnergyTimeStamp *end)
{
    if (OPT_UNLIKELY(begin == NULL))
    {
        perror("Error: Begin timestamp is NULL \n");
        return NULL;
    }
    if (OPT_UNLIKELY(end == NULL))
    {
        perror("Error: End timestamp is NULL\n");
        return NULL;
    }

    // Check if both a64fx arrays are compatible
    if (OPT_UNLIKELY((begin->a64fx_counter_name == NULL) != (end->a64fx_counter_name == NULL) || (begin->a64fx_counter_value == NULL) != (end->a64fx_counter_value == NULL) || begin->a64fx_arr_size != end->a64fx_arr_size))
    {
        perror("Error: a64fx arrays are incompatible in A64FX measurements\n");
        return NULL;
    }

    // Check if both rapl arrays are compatible
    if (OPT_UNLIKELY((begin->rapl_counter_name == NULL) != (end->rapl_counter_name == NULL) || (begin->rapl_counter_value == NULL) != (end->rapl_counter_value == NULL) || begin->rapl_arr_size != end->rapl_arr_size))
    {
        perror("Error: rapl arrays are incompatible in RAPL measurements\n");
        return NULL;
    }

    // Check if both nvml arrays are compatible
    if (OPT_UNLIKELY((begin->nvml_counter_name == NULL) != (end->nvml_counter_name == NULL) || (begin->nvml_counter_value == NULL) != (end->nvml_counter_value == NULL) || begin->nvml_arr_size != end->nvml_arr_size))
    {
        perror("Error: nvml arrays are incompatible in NVML measurements\n");
        return NULL;
    }

    // Check if both rocm arrays are compatible
    if (OPT_UNLIKELY((begin->rocm_counter_name == NULL) != (end->rocm_counter_name == NULL) || (begin->rocm_counter_value == NULL) != (end->rocm_counter_value == NULL) || begin->rocm_arr_size != end->rocm_arr_size))
    {
        perror("Error: rocm arrays are incompatible in ROCM measurements\n");
        return NULL;
    }

    // Check if both hdeem arrays are compatible
    if (OPT_UNLIKELY((begin->hdeem_counter_name == NULL) != (end->hdeem_counter_name == NULL) || (begin->hdeem_counter_value == NULL) != (end->hdeem_counter_value == NULL) || begin->hdeem_arr_size != end->hdeem_arr_size))
    {
        perror("Error: hdeem arrays are incompatible in HDEEM measurements\n");
        return NULL;
    }

    struct MidlibEnergyTimeStamp *result = initEnergyTimeStamp();

    if (OPT_LIKELY(begin->a64fx_counter_value != NULL && begin->a64fx_counter_name != NULL))
    {
        int cumulative = 0;
        result->a64fx_arr_size = begin->a64fx_arr_size;
        result->a64fx_energy_per_counter = (double *)malloc(begin->a64fx_arr_size * sizeof(double));
        for (size_t i = 0; i < begin->a64fx_arr_size; i++)
        {
            result->a64fx_energy_per_counter[i] = meric::A64FX::getResultValue(begin->a64fx_counter_value[i], end->a64fx_counter_value[i], (end->id - begin->id), cumulative, begin->a64fx_counter_name[i]);
            result->a64fx_energy_total += result->a64fx_energy_per_counter[i];
        }
    }
    if (OPT_LIKELY(begin->rapl_counter_value != NULL && begin->rapl_counter_name != NULL))
    {
        int cumulative = 0;
        result->rapl_arr_size = begin->rapl_arr_size;
        result->rapl_energy_per_counter = (double *)malloc(begin->rapl_arr_size * sizeof(double));
        for (size_t i = 0; i < begin->rapl_arr_size; i++)
        {
            result->rapl_energy_per_counter[i] = meric::RAPL::getResultValue(begin->rapl_counter_value[i], end->rapl_counter_value[i], (end->id - begin->id), cumulative, begin->rapl_counter_name[i]);
            result->rapl_energy_total += result->rapl_energy_per_counter[i];
        }
    }
    if (OPT_LIKELY(begin->nvml_counter_value != NULL && begin->nvml_counter_name != NULL))
    {
        int cumulative = 0;
        result->nvml_arr_size = begin->nvml_arr_size;
        result->nvml_energy_per_counter = (double *)malloc(begin->nvml_arr_size * sizeof(double));
        for (size_t i = 0; i < begin->nvml_arr_size; i++)
        {
            result->nvml_energy_per_counter[i] = meric::NVML::getResultValue(begin->nvml_counter_value[i], end->nvml_counter_value[i], (end->id - begin->id), cumulative, begin->nvml_counter_name[i]);
            result->nvml_energy_total += result->nvml_energy_per_counter[i];
        }
    }
    // if (OPT_LIKELY(begin->rocm_counter_value != NULL && begin->rocm_counter_name != NULL))
    // {
    //     int cumulative = 0;
    //     result->rocm_arr_size = begin->rocm_arr_size;
    //     result->rocm_energy_per_counter = (double *)malloc(begin->rocm_arr_size * sizeof(double));
    //     for (size_t i = 0; i < begin->rocm_arr_size; i++)
    //     {
    //         result->rocm_energy_per_counter[i] = meric::ROCM::getResultValue(begin->rocm_counter_value[i], end->rocm_counter_value[i], (end->id - begin->id), cumulative, begin->rocm_counter_name[i]);
    //         result->rocm_energy_total += result->rocm_energy_per_counter[i];
    //     }
    // }
    // if (OPT_LIKELY(begin->hdeem_counter_value != NULL && begin->hdeem_counter_name != NULL))
    // {
    //     int cumulative = 0;
    //     result->hdeem_arr_size = begin->hdeem_arr_size;
    //     result->hdeem_energy_per_counter = (double *)malloc(begin->hdeem_arr_size * sizeof(double));
    //     for (size_t i = 0; i < begin->hdeem_arr_size; i++)
    //     {
    //         result->hdeem_energy_per_counter[i] = meric::HDEEM::getResultValue(begin->hdeem_counter_value[i], end->hdeem_counter_value[i], (end->id - begin->id), cumulative, begin->hdeem_counter_name[i], "");
    //         result->hdeem_energy_total += result->hdeem_energy_per_counter[i];
    //     }
    // }

    result->energy_total = result->a64fx_energy_total + result->rapl_energy_total + result->nvml_energy_total + result->rocm_energy_total + result->hdeem_energy_total;

    return result;
}

char *midlib_get_enabled_domain_names(MidlibEnergy *enabled_domains)
{
    std::string buffer = "";

    // Check if no domains are selected
    if (OPT_UNLIKELY(MIDLIB_ENERGY_NO_SELECTION(*enabled_domains)))
    {
        buffer = "NO_SELECTION";
    }
    else
    {
        // Append domain names for matches
        if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_A64FX)))
        {
            buffer += "A64FX, ";
        }
        if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_RAPL)))
        {
            buffer += "RAPL, ";
        }
        if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_NVML)))
        {
            buffer += "NVML, ";
        }
        if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_ROCM)))
        {
            buffer += "ROCM, ";
        }
        if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_HDEEM)))
        {
            buffer += "HDEEM, ";
        }
    }

    // Remove the trailing ", " if present
    if (!buffer.empty() && buffer.back() == ' ')
    {
        buffer.pop_back(); // Remove the last space
        buffer.pop_back(); // Remove the last comma
    }

    char *c_str = (char *)malloc(buffer.size() + 1); // +1 for null terminator
    std::copy(buffer.begin(), buffer.end(), c_str);
    c_str[buffer.size()] = '\0'; // Null-terminate the string
    return c_str;
}
