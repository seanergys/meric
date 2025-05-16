#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <omp.h>
#include <stdbool.h>

#include "meric_ext.h"
#include "../basis/utils.h"
#include "../basis/cpu_optimisations.h"
#include "../wrapper/environmentwrapper.h"
#include "../wrapper/a64fxwrapper.h"
#include "../wrapper/raplwrapper.h"
#include "../wrapper/nvmlwrapper.h"
#include "../wrapper/rocmwrapper.h"
#include "../wrapper/hdeemwrapper.h"
#include "../wrapper/hwmonwrapper.h"

#include "../basis/cpu_optimisations.h"


/*
    --------- Compilation Unit Only Method Declerations ---------

    These methods are not present in the header file &
    may or maynot be called indirectly by the extlib_* methods

*/
struct ExtlibEnergyTimeStamp *initEnergyTimeStamp();
static const char *printDomainName(unsigned int value);

template <typename Tmode> static uint64_t *measureEnergy(size_t *size, double timeNow = 0.0);
template <typename Tmode> static void close();
template <typename Tmode> static void printArray(const char *name, Tmode *arr, size_t size, size_t print_limit);

/* --------------------------------------------------------------- */

template <typename Tmode, typename TmapVal>
static void measureEnergy(size_t *size, uint64_t **values, char ***labels)
{
    std::map<std::string, TmapVal> energyNow;
    Tmode::getCounterValues(energyNow);

    *size = energyNow.size();
    *values = (uint64_t *)malloc(*size * sizeof(uint64_t));
    *labels = (char **)malloc(*size * sizeof(char *));

    if (!(*values) || !(*labels))
    {
        std::cerr << "[ERROR]: Memory allocation failed!\n";
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

template static void measureEnergy<meric::A64FX, unsigned long long int>(size_t *, uint64_t **, char ***);
template static void measureEnergy<meric::RAPL, unsigned long long int>(size_t *, uint64_t **, char ***);
template static void measureEnergy<meric::NVML, unsigned long long int>(size_t *, uint64_t **, char ***);
template static void measureEnergy<meric::HWMON, unsigned long long int>(size_t *, uint64_t **, char ***);
template static void measureEnergy<meric::ROCM, uint64_t>(size_t *, uint64_t **, char ***);
template static void measureEnergy<meric::HDEEM, double>(size_t *, uint64_t **, char ***);

template <typename Tmode>
static void close()
{
    Tmode::close();
}
template static void close<meric::A64FX>();
template static void close<meric::RAPL>();
template static void close<meric::ROCM>();
template static void close<meric::HDEEM>();
template static void close<meric::HWMON>();
template <>
void close<meric::NVML>()
{
#ifdef HAVE_NVML_H
    nvmlShutdown();
    meric::NVML::close();
#endif
}

template <typename Tmode>
static void printArray(const char *name, Tmode *arr, size_t size, size_t print_limit)
{
    printf("| %-15s (size: %zu): ", name, size);

    if (!arr || size == 0)
    {
        printf("NULL or empty array\n");
        return;
    }

    printf("[");

    print_limit = (print_limit == static_cast<size_t>(-1)) ? size : print_limit;
    for (size_t i = 0; i < size && i < print_limit; ++i)
    {
        if (std::is_same<Tmode, double>::value)
        {
            printf("%f", arr[i]); // Special format for doubles
        }
        else if (std::is_same<Tmode, uint64_t>::value)
        {
            printf("%lu", arr[i]); // Format for uint64_t
        }
        else if (std::is_same<Tmode, char *>::value)
        {
            printf("%s", arr[i]); // format for char* (string)
        }

        if (i < size - 1 && i < print_limit - 1)
        {
            printf(", ");
        }
    }

    if (size > print_limit)
    {
        printf(", ..."); // Indicate truncated output
    }

    printf("]\n");
}

template static void printArray<double>(const char *, double *, size_t, size_t);
template static void printArray<uint64_t>(const char *, uint64_t *, size_t, size_t);
template static void printArray<char *>(const char *, char **, size_t, size_t);

void extlib_init(ExtlibEnergy *enabled_domains, bool is_detailed)
{
    if (OPT_UNLIKELY(EXTLIB_ENERGY_NO_SELECTION(*enabled_domains)))
    {
        perror("[ERROR]: No Domain Selected!\n");
        return;
    }

    meric::Environment::initHWSizes();

    for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        int index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = NULL;
        enabled_domains->closeFns[index] = NULL;
    }

    if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX)))
    {
#if defined HAVE_LINUX_PERF_EVENT_H && defined __aarch64__
        meric::A64FX::init(is_detailed);
        int index = meric::mutils::countTrailingZeros(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::A64FX, unsigned long long int>;
        enabled_domains->closeFns[index] = close<meric::A64FX>;
        printf("[INFO]: Domain %s is init [OK]\n", printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX));
#else
        fprintf(stderr, "[ERROR]: Turning off %s. It is selected but does NOT exists on this machine!\n", printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX));
        EXTLIB_ENERGY_DISABLE_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX);
#endif
    }
    if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL)))
    {
#if defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_GEOPM_H
        meric::RAPL::init(is_detailed);
        meric::RAPL::initCountersUnits();
        int index = meric::mutils::countTrailingZeros(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::RAPL, unsigned long long int>;
        enabled_domains->closeFns[index] = close<meric::RAPL>;
        printf("[INFO]: Domain %s is init [OK]\n", printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL));
#else
        fprintf(stderr, "[ERROR]: Turning off %s. It is selected but does NOT exists on this machine!\n", printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL));
        EXTLIB_ENERGY_DISABLE_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL);
#endif
    }
    if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML)))
    {
#ifdef HAVE_NVML_H
        nvmlInit();
        meric::NVML::init(is_detailed);
        int index = meric::mutils::countTrailingZeros(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML); // Get index of the enabled domain
        // std::cout << "index =" << index << " gpus-per-node:" << meric::NVML::getGPUsPerNode() << "\n";

        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::NVML, unsigned long long int>;
        enabled_domains->closeFns[index] = close<meric::NVML>;
        printf("[INFO]: Domain %s is init [OK]\n", printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML));
#else
        fprintf(stderr, "[ERROR]: Turning off %s. It is selected but does NOT exists on this machine!\n", printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML));
        EXTLIB_ENERGY_DISABLE_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML);
#endif
    }

    if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM)))
    {
#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
        meric::ROCM::init(is_detailed);
        int index = meric::mutils::countTrailingZeros(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::ROCM, uint64_t>;
        enabled_domains->closeFns[index] = close<meric::ROCM>;
        printf("[INFO]: Domain %s is init [OK]\n", printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM));
#else
        fprintf(stderr, "[ERROR]: Turning off %s. It is selected but does NOT exists on this machine!\n", printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM));
        EXTLIB_ENERGY_DISABLE_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM);

#endif
    }
    if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM)))
    {
#ifdef HAVE_HDEEM_H
        meric::HDEEM::init(is_detailed);
        int index = meric::mutils::countTrailingZeros(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::HDEEM, double>;
        enabled_domains->closeFns[index] = close<meric::HDEEM>;
        printf("[INFO]: Domain %s is init [OK]\n", printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM));
#else
        fprintf(stderr, "[ERROR]: Turning off %s. It is selected but does NOT exists on this machine!\n", printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM));
        EXTLIB_ENERGY_DISABLE_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM);
#endif
    }

    if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON)))
    {
        meric::HWMON::init(is_detailed);
        int index = meric::mutils::countTrailingZeros(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::HWMON, double>;
        enabled_domains->closeFns[index] = close<meric::HWMON>;
        printf("[INFO]: Domain %s is init [OK]\n", printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON));
    }
}

static const char *printDomainName(unsigned int value)
{
    switch (value)
    {
    case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX:
        return "A64FX";
        break;
    case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL:
        return "RAPL";
        break;
    case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML:
        return "NVML";
        break;
    case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM:
        return "ROCM";
        break;
    case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM:
        return "HDEEM";
        break;
    case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON:
        return "HWMON";
        break;
    case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END:
        return "END";
        break;
    case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NO_SELECTION:
        return "NO_SELECTION";
        break;
    default:
        return "Unknown Domain";
        break;
    }
}

void extlib_close(ExtlibEnergy *enabled_domains)
{
    for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain)) // Check if the domain is enabled
        {
            int index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
            if (enabled_domains->closeFns[index])
            {
                enabled_domains->closeFns[index]();
                printf("[INFO]: Domain %s is closed [OK]\n", printDomainName(domain));
            }
        }
    }
}

ExtlibEnergyTimeStamp *extlib_read_energy_measurements(ExtlibEnergy *enabled_domains)
{
    struct ExtlibEnergyTimeStamp *timestamp = initEnergyTimeStamp();

    for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain))) // Check if the domain is enabled
        {
            int index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
            if (OPT_LIKELY(enabled_domains->energyMeasurementFns[index]))
            {
                size_t arr_size;
                uint64_t *counter_value;
                char **counter_label;
                enabled_domains->energyMeasurementFns[index](&arr_size, &counter_value, &counter_label);

                timestamp->domain_data[index].domain_id = domain;
                timestamp->domain_data[index].arr_size = arr_size;
                timestamp->domain_data[index].counter_name = counter_label;
                timestamp->domain_data[index].counter_value = counter_value;
                printf("[INFO]: Energy is read from Domain %s [OK]\n", printDomainName(domain));
            }
            else
                printf("[INFO]: ExtlibEnergy is NOT initialised correctly! Domain %s's energyMeasurementFns is NOT set! \n", printDomainName(domain));
        }
         
    }
    return timestamp;
}

struct ExtlibEnergyTimeStamp *initEnergyTimeStamp()
{
    struct ExtlibEnergyTimeStamp *energy_timestamp = (struct ExtlibEnergyTimeStamp *)malloc(sizeof(struct ExtlibEnergyTimeStamp));
    if (OPT_UNLIKELY(energy_timestamp == NULL))
    {
        perror("[ERROR]: Failed to allocate memory for the EnergyTimeStamp");
        exit(EXIT_FAILURE);
    }

    memset(energy_timestamp, 0, sizeof(struct ExtlibEnergyTimeStamp)); // Correct: clears struct fields

    // set domain id, this is important for printing the structure (otherwise no_domain_selected is printed)
    for (size_t i = 0; i < EXTLIB_NUM_DOMAINS; i++)
        energy_timestamp->domain_data[i].domain_id = 1 << i;

    struct timespec tt;
    clock_gettime(CLOCK_REALTIME, &tt);
    energy_timestamp->id = meric::mutils::timeToDouble(tt);
    return energy_timestamp;
}

void extlib_print_energy_timestamp(struct ExtlibEnergyTimeStamp *timestamp, size_t print_limit)
{

#define MAX_DOMAIN_NAME_LENGTH 50 // Adjusted for "NO_SELECTION" and "Unknown Domain"
    if (OPT_UNLIKELY(timestamp == NULL))
    {
        perror("[ERROR]:  NULL timestamp\n");
        return;
    }
    // Print pointer addresses and array sizes
    printf("------- ExtlibEnergyTimeStamp (id: %.4f) -------\n", timestamp->id);

    for (int i = 0; i < EXTLIB_NUM_DOMAINS; i++)
    {
        char domain_name_buffer[MAX_DOMAIN_NAME_LENGTH];

        const char *domain_name = printDomainName(timestamp->domain_data[i].domain_id);

        snprintf(domain_name_buffer, sizeof(domain_name_buffer), "%s_counter_name", domain_name);
        printArray(domain_name_buffer, timestamp->domain_data[i].counter_name, timestamp->domain_data[i].arr_size, print_limit);

        snprintf(domain_name_buffer, sizeof(domain_name_buffer), "%s_counter_value", domain_name);
        printArray(domain_name_buffer, timestamp->domain_data[i].counter_value, timestamp->domain_data[i].arr_size, print_limit);

        snprintf(domain_name_buffer, sizeof(domain_name_buffer), "%s_energy_per_counter", domain_name);
        printArray(domain_name_buffer, timestamp->domain_data[i].energy_per_counter, timestamp->domain_data[i].arr_size, print_limit);
        printf("| %s_total_energy: %lf\n", domain_name, timestamp->domain_data[i].energy_total);
        printf("|\n");
    }
    printf("| Total_energy consumption: %lf [J]\n", timestamp->energy_total);
    printf("-----------------------------------\n");

#undef MAX_DOMAIN_NAME_LENGTH
}

void extlib_free_energy_timestamp(struct ExtlibEnergyTimeStamp *timestamp)
{

    if (timestamp == NULL)
    {
        return; // Nothing to free
    }

    // Iterate over each domain
    for (int i = 0; i < EXTLIB_NUM_DOMAINS; i++)
    {
        // Free the counter_name array (array of strings)
        if (OPT_LIKELY(timestamp->domain_data[i].counter_name != NULL))
        {
            for (size_t j = 0; j < timestamp->domain_data[i].arr_size; j++)
            {
                free(timestamp->domain_data[i].counter_name[j]); // Free each string
            }
            free(timestamp->domain_data[i].counter_name); // Free the array of pointers
        }

        // Free the counter_value array
        if (OPT_LIKELY(timestamp->domain_data[i].counter_value != NULL))
        {
            free(timestamp->domain_data[i].counter_value);
        }

        // Free the energy_per_counter array
        if (OPT_LIKELY(timestamp->domain_data[i].energy_per_counter != NULL))
        {
            free(timestamp->domain_data[i].energy_per_counter);
        }
    }

    // Finally, free the entire timestamp structure
    free(timestamp);
}

struct ExtlibEnergyTimeStamp *extlib_calc_energy_consumption(struct ExtlibEnergyTimeStamp *begin, struct ExtlibEnergyTimeStamp *end)
{
    if (OPT_UNLIKELY(begin == NULL))
    {
        perror("[ERROR]:  Begin timestamp is NULL \n");
        return NULL;
    }
    if (OPT_UNLIKELY(end == NULL))
    {
        perror("[ERROR]:  End timestamp is NULL\n");
        return NULL;
    }

    // Check if all domain arrays are compatible
    for (int i = 0; i < EXTLIB_NUM_DOMAINS; i++)
    {
        if (OPT_UNLIKELY((begin->domain_data[i].counter_name == NULL) != (end->domain_data[i].counter_name == NULL) ||
                         (begin->domain_data[i].counter_value == NULL) != (end->domain_data[i].counter_value == NULL) ||
                         begin->domain_data[i].arr_size != end->domain_data[i].arr_size))
        {
            fprintf(stderr, "[ERROR]: Arrays for domain %d are incompatible\n", printDomainName(begin->domain_data[i].domain_id));
            return NULL;
        }
    }

    struct ExtlibEnergyTimeStamp *result = initEnergyTimeStamp();

    // Calculate energy consumption for each domain
    for (int i = 0; i < EXTLIB_NUM_DOMAINS; i++)
    {
        if (OPT_LIKELY(begin->domain_data[i].counter_value != NULL && begin->domain_data[i].counter_name != NULL))
        {
            result->domain_data[i].arr_size = begin->domain_data[i].arr_size;
            result->domain_data[i].energy_per_counter = (double *)malloc(begin->domain_data[i].arr_size * sizeof(double));
            if (OPT_UNLIKELY(result->domain_data[i].energy_per_counter == NULL))
            {
                perror("[ERROR]: Failed to allocate memory for energy_per_counter");
                extlib_free_energy_timestamp(result); // Free any previously allocated memory
                return NULL;
            } 
 
            result->domain_data[i].counter_name = (char **)malloc(begin->domain_data[i].arr_size * sizeof(char *));
            if (OPT_UNLIKELY(result->domain_data[i].counter_name == NULL))
            {
                perror("[ERROR]: Failed to allocate memory for the EnergyTimeStamp.counter_name");
                exit(EXIT_FAILURE);
            }

            for (size_t j = 0; j < begin->domain_data[i].arr_size; j++)
            {
                result->domain_data[i].counter_name[j] = strdup(begin->domain_data[i].counter_name[j]);
                if (!result->domain_data[i].counter_name[j])
                {
                    fprintf(stderr, "[ERROR]: Failed to allocate memory for the EnergyTimeStamp.counter_name[%d]", j);
                    exit(EXIT_FAILURE);
                }
            }

            int cumulative = 0;
            for (size_t j = 0; j < begin->domain_data[i].arr_size; j++)
            {
                // Call the appropriate energy calculation function based on the domain
                switch (begin->domain_data[i].domain_id)
                {

                case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX:
                    cumulative = 0;
                    result->domain_data[i].energy_per_counter[j] = meric::A64FX::getResultValue(
                        begin->domain_data[i].counter_value[j], end->domain_data[i].counter_value[j],
                        (end->id - begin->id), cumulative, begin->domain_data[i].counter_name[j]);
                    break;

                case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL:
                    cumulative = 1;
                    result->domain_data[i].energy_per_counter[j] = meric::RAPL::getResultValue(
                        begin->domain_data[i].counter_value[j], end->domain_data[i].counter_value[j],
                        (end->id - begin->id), cumulative, begin->domain_data[i].counter_name[j]);
                    break;

                case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML:
                    cumulative = 0;
                    result->domain_data[i].energy_per_counter[j] = meric::NVML::getResultValue(
                        begin->domain_data[i].counter_value[j], end->domain_data[i].counter_value[j],
                        (end->id - begin->id), cumulative, begin->domain_data[i].counter_name[j]);
                    break;

                case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM:
                    result->domain_data[i].energy_per_counter[j] = meric::ROCM::getResultValue(
                        begin->domain_data[i].counter_value[j], end->domain_data[i].counter_value[j],
                        (end->id - begin->id), cumulative, begin->domain_data[i].counter_name[j]);
                    break;

                case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM:
                    result->domain_data[i].energy_per_counter[j] = meric::HDEEM::getResultValue(
                        begin->domain_data[i].counter_value[j], end->domain_data[i].counter_value[j],
                        (end->id - begin->id), cumulative, begin->domain_data[i].counter_name[j], "");
                    break;

                case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON:
                    cumulative = 0;
                    result->domain_data[i].energy_per_counter[j] = meric::HWMON::getResultValue(
                        begin->domain_data[i].counter_value[j], end->domain_data[i].counter_value[j],
                        (end->id - begin->id), cumulative, begin->domain_data[i].counter_name[j]);
                    break;

                default:
                    fprintf(stderr, "[ERROR]: Unknown domain ID %d\n", begin->domain_data[i].domain_id);
                }
                result->domain_data[i].energy_total += result->domain_data[i].energy_per_counter[j];
            }
            result->energy_total += result->domain_data[i].energy_total;
        }
    }

    return result;
}

char *extlib_get_enabled_domain_names(ExtlibEnergy *enabled_domains)
{
    std::string buffer = "";

    // Check if no domains are selected
    if (OPT_UNLIKELY(EXTLIB_ENERGY_NO_SELECTION(*enabled_domains)))
    {
        buffer = "NO_SELECTION";
    }
    else
    {
        // Append domain names for matches
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX)))
        {
            buffer += "A64FX, ";
        }
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL)))
        {
            buffer += "RAPL, ";
        }
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML)))
        {
            buffer += "NVML, ";
        }
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM)))
        {
            buffer += "ROCM, ";
        }
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM)))
        {
            buffer += "HDEEM, ";
        }
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON)))
        {
            buffer += "HWMON, ";
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
