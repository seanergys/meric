#include <vector>
#include <map>
#include <unordered_map>
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

#define MAX_DOMAIN_NAME_LENGTH 50
/*
    --------- Compilation Unit Only Variable Declerations ---------

    These methods are not present in the header file &
    may or maynot be called indirectly by the extlib_* methods

*/
struct DomainMemoryMap
{
    char **counter_name = nullptr;
    uint64_t *counter_value = nullptr;
    double *energy_per_counter = nullptr;
    size_t arr_size = 0;
};
static std::unordered_map<int, DomainMemoryMap> domain_pointer_map;
static std::vector<ExtlibEnergyTimeStamp> static_memory;
static size_t static_memory_size; // in num of elements
static bool use_static_memory = false;

/*
    --------- Compilation Unit Only Method Declerations ---------

    These methods are not present in the header file &
    may or maynot be called indirectly by the extlib_* methods

*/
struct ExtlibEnergyTimeStamp *initEnergyTimeStamp();
static const char *printDomainName(unsigned int value);

template <typename Tmode>
static uint64_t *measureEnergy(size_t *size, double timeNow = 0.0);
template <typename Tmode>
static void close();
template <typename Tmode>
static void printArray(const char *name, Tmode *arr, size_t size, size_t print_limit);

/* --------------------------------------------------------------- */

template <typename Tmode, typename TmapVal>
static void measureEnergy(size_t *size, uint64_t **values, char ***labels, size_t static_memory_index, size_t domain_index)
{
    std::map<std::string, TmapVal> energyNow;
    Tmode::getCounterValues(energyNow);

    *size = energyNow.size();

    if (use_static_memory)
    {
        DomainMemoryMap &map = domain_pointer_map.at(domain_index);
        size_t arr_size = map.arr_size;

        // Safety check
        if (*size > arr_size)
        {
            fprintf(stderr, "[WARN]: Measurement size (%zu) exceeds allocated arr_size (%zu) for domain %d\n", *size, arr_size, domain_index);
            exit(EXIT_FAILURE);
        }

        size_t offset = static_memory_index * arr_size;
        // std::cout << "offset is ->" << offset << "\n";

        *values = &map.counter_value[offset];
        *labels = map.counter_name;

        size_t i = 0;
        for (const auto &entry : energyNow)
        {
            if (i >= arr_size)
                break;

            (*values)[i] = static_cast<uint64_t>(entry.second);
            // strncpy((*labels)[i], entry.first.c_str(), MAX_DOMAIN_NAME_LENGTH - 1);
            // (*labels)[i][MAX_DOMAIN_NAME_LENGTH - 1] = '\0';
            ++i;
        }
    }
    else
    {
        // Dynamic allocation path
        *values = (uint64_t *)malloc(*size * sizeof(uint64_t));
        *labels = (char **)malloc(*size * sizeof(char *));

        if (!(*values) || !(*labels))
        {
            std::cerr << "[ERROR]: Memory allocation failed in dynamic mode!\n";
            return;
        }

        size_t i = 0;
        for (const auto &entry : energyNow)
        {
            (*values)[i] = static_cast<uint64_t>(entry.second);
            (*labels)[i] = (char *)malloc(MAX_DOMAIN_NAME_LENGTH);
            if ((*labels)[i])
            {
                strncpy((*labels)[i], entry.first.c_str(), MAX_DOMAIN_NAME_LENGTH - 1);
                (*labels)[i][MAX_DOMAIN_NAME_LENGTH - 1] = '\0';
            }
            ++i;
        }
    }
}

template static void measureEnergy<meric::A64FX, unsigned long long int>(size_t *, uint64_t **, char ***, size_t, size_t);
template static void measureEnergy<meric::RAPL, unsigned long long int>(size_t *, uint64_t **, char ***, size_t, size_t);
template static void measureEnergy<meric::NVML, unsigned long long int>(size_t *, uint64_t **, char ***, size_t, size_t);
template static void measureEnergy<meric::HWMON, unsigned long long int>(size_t *, uint64_t **, char ***, size_t, size_t);
template static void measureEnergy<meric::ROCM, uint64_t>(size_t *, uint64_t **, char ***, size_t, size_t);
template static void measureEnergy<meric::HDEEM, double>(size_t *, uint64_t **, char ***, size_t, size_t);

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

#if defined HAVE_SYSFS_RAPL || defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_GEOPM_H || HAVE_LINUX_PERF_EVENT_H
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
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::HWMON, unsigned long long int>;
        enabled_domains->closeFns[index] = close<meric::HWMON>;
        printf("[INFO]: Domain %s is init [OK]\n", printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON));
    }

    // This executes only once.
    // It will perform a read as normal (using dynamic memory) to see how many fields will return
    // based on the size of the fields, we allocate a chunk of memory to be used.
    if (enabled_domains->use_static_memory)
    {
        static_memory_size = enabled_domains->static_memory_size;
        static_memory.reserve(static_memory_size); // reserve memory
        printf("[INFO]: Using static memory for %zu elements. [OK]\n", static_memory_size);

        // make a read to see domain array sizes.
        struct ExtlibEnergyTimeStamp *temp = extlib_read_energy_measurements(enabled_domains);

        for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
        {
            if (!EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain)) // Check if the domain is enabled
                continue;                                            // Skip unused domains

            int domain_index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain

            DomainMemoryMap map;
            map.arr_size = temp->domain_data[domain_index].arr_size;
            size_t total_elements = map.arr_size * static_memory_size; // this will store whole records.

            // Allocate flat arrays for all timestamps
            map.counter_value = (uint64_t *)calloc(total_elements, sizeof(uint64_t));  // allocate space for each sample
            map.energy_per_counter = (double *)calloc(total_elements, sizeof(double)); // allocate space for each sample.
            map.counter_name = (char **)calloc(map.arr_size, sizeof(char *));          // allocate space for ONE sample, since all samples will be the same.

            if (!map.counter_value || !map.energy_per_counter || !map.counter_name)
            {
                perror("[ERROR]: Failed to allocate static memory for domain arrays");
                exit(EXIT_FAILURE);
            }

            // Allocate strings for every label slot across all timestamps

            for (size_t j = 0; j < map.arr_size; j++)
            {
                map.counter_name[j] = (char *)calloc(MAX_DOMAIN_NAME_LENGTH, sizeof(char));
                if (!map.counter_name[j])
                {
                    perror("[ERROR]: Failed to allocate memory for counter_name string");
                    exit(EXIT_FAILURE);
                }

                strncpy(map.counter_name[j], temp->domain_data[domain_index].counter_name[j], MAX_DOMAIN_NAME_LENGTH - 1);
                map.counter_name[j][MAX_DOMAIN_NAME_LENGTH - 1] = '\0';
            }

            domain_pointer_map[domain_index] = std::move(map);
        }

        extlib_free_energy_timestamp(temp);

        // IMPORTANT: DO NOT CHANGE THIS!
        // I set this at the end, since I need a normal extlib_read_energy_measurements() read at the beginning (no static_map available at that time.) I first need the numbers.
        use_static_memory = true;
        // std::cout << "extlib_init set use_static_memory:" << use_static_memory << "\n";
    }
    else
    {
        printf("[INFO]: Using dynamic memory. [OK]\n");
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
        if (EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain))
        {
            int index = meric::mutils::countTrailingZeros(domain);
            if (use_static_memory && domain_pointer_map.count(index))
            {
                DomainMemoryMap &dm = domain_pointer_map[index];
                if (dm.counter_name)
                {
                    for (size_t i = 0; i < dm.arr_size; ++i)
                        if (dm.counter_name[i])
                            free(dm.counter_name[i]);
                    free(dm.counter_name);
                    dm.counter_name = nullptr;
                }
                if (dm.counter_value)
                {
                    free(dm.counter_value);
                    dm.counter_value = nullptr;
                }
                if (dm.energy_per_counter)
                {
                    free(dm.energy_per_counter);
                    dm.energy_per_counter = nullptr;
                }
            }

            if (enabled_domains->closeFns[index])
            {
                enabled_domains->closeFns[index]();
                printf("[INFO]: Domain %s is closed [OK]\n", printDomainName(domain));
            }
        }
    }

    domain_pointer_map.clear();
    static_memory.clear();
    use_static_memory = false;
}

ExtlibEnergyTimeStamp *extlib_read_energy_measurements(ExtlibEnergy *enabled_domains)
{
    struct ExtlibEnergyTimeStamp *timestamp = initEnergyTimeStamp();

    for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain))) // Check if the domain is enabled
        {
            int domain_index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
            if (OPT_LIKELY(enabled_domains->energyMeasurementFns[domain_index]))
            {

                // std::cout << "extlib_read_energy_measurements-> reading for dynamic memory\n";
                timestamp->domain_data[domain_index].domain_id = domain;
                enabled_domains->energyMeasurementFns[domain_index](
                    &timestamp->domain_data[domain_index].arr_size,
                    &timestamp->domain_data[domain_index].counter_value,
                    &timestamp->domain_data[domain_index].counter_name,
                    timestamp->static_memory_index,
                    domain_index);

                printf("[INFO]: Energy is read from Domain %s [OK]\n", printDomainName(domain));
            }
            else
                printf("[INFO]: ExtlibEnergy is NOT initialised correctly! Domain %s's energyMeasurementFns is NOT set! \n", printDomainName(domain));
        }
    }
    return timestamp;
}

/**
 * if use_static_memory, try to add to the end of the vector.
 * if you reach the end, check whole vector for a free memory location. if not found, exit with error.
 */
struct ExtlibEnergyTimeStamp *initEnergyTimeStamp()
{
    // std::cout << "initEnergyTimeStamp()\n";
    if (use_static_memory)
    {
        if (static_memory.size() + 1 > static_memory_size)
        {
            // Search for reusable slot (id == 0 means previously cleared)
            for (size_t i = 0; i < static_memory.size(); ++i)
            {
                if (static_memory[i].id == 0)
                {
                    std::cout << "reusing slot!\n";
                    struct ExtlibEnergyTimeStamp *ptr = &static_memory[i];
                    std::memset(ptr, 0, sizeof(struct ExtlibEnergyTimeStamp));

                    ptr->static_memory_index = i;
                    std::cout << "memory index is set to:" << ptr->static_memory_index << "\n";

                    for (size_t i = 0; i < EXTLIB_NUM_DOMAINS; ++i)
                        ptr->domain_data[i].domain_id = (1 << i);

                    struct timespec tt;
                    clock_gettime(CLOCK_REALTIME, &tt);
                    ptr->id = meric::mutils::timeToDouble(tt);

                    return ptr;
                }
            }
            // If no reusable slot found
            std::cerr << "[ERR]: static memory exhausted, and no reusable slot found!\n";
            exit(EXIT_FAILURE);
        }
        else
        {
            std::cout << "pushed back!\n";
            static_memory.emplace_back();
            struct ExtlibEnergyTimeStamp *ptr = &static_memory.back();
            std::memset(ptr, 0, sizeof(struct ExtlibEnergyTimeStamp));

            ptr->static_memory_index = static_memory.size() - 1;
            std::cout << "memory index is set to:" << ptr->static_memory_index << "\n";

            for (size_t i = 0; i < EXTLIB_NUM_DOMAINS; ++i)
                ptr->domain_data[i].domain_id = (1 << i);

            struct timespec tt;
            clock_gettime(CLOCK_REALTIME, &tt);
            ptr->id = meric::mutils::timeToDouble(tt);
            return ptr;
        }
    }
    else
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
            energy_timestamp->domain_data[i].domain_id = (1 << i);

        struct timespec tt;
        clock_gettime(CLOCK_REALTIME, &tt);
        energy_timestamp->id = meric::mutils::timeToDouble(tt);
        return energy_timestamp;
    }
}

void extlib_print_energy_timestamp(struct ExtlibEnergyTimeStamp *timestamp, size_t print_limit)
{

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
}

void extlib_free_energy_timestamp(struct ExtlibEnergyTimeStamp *timestamp)
{

    if (timestamp == NULL)
        return;

    if (!use_static_memory)
    {
        // Iterate over each domain
        for (int i = 0; i < EXTLIB_NUM_DOMAINS; i++)
        {
            // Free the counter_name array (array of strings)
            if (OPT_LIKELY(timestamp->domain_data[i].counter_name != NULL))
            {
                for (size_t j = 0; j < timestamp->domain_data[i].arr_size; j++)
                {
                    if (timestamp->domain_data[i].counter_name[j] != NULL)
                    {
                        free(timestamp->domain_data[i].counter_name[j]); // Free each string
                        timestamp->domain_data[i].counter_name[j] = NULL;
                    }
                }
                free(timestamp->domain_data[i].counter_name); // Free the array of pointers
                timestamp->domain_data[i].counter_name = NULL;
            }
            // Free the counter_value array
            if (OPT_LIKELY(timestamp->domain_data[i].counter_value != NULL))
            {
                free(timestamp->domain_data[i].counter_value);
                timestamp->domain_data[i].counter_value = NULL;
            }
            // Free the energy_per_counter array
            if (OPT_LIKELY(timestamp->domain_data[i].energy_per_counter != NULL))
            {
                free(timestamp->domain_data[i].energy_per_counter);
                timestamp->domain_data[i].energy_per_counter = NULL;
            }
        }
        free(timestamp); // Heap-allocated
    }
    else
    {
        // std::cout << "extlib_free_energy_timestamp -> static_memory_erase\n";
        // IMPORTANT: do not erase the object since it may cause memory re-allocation of remanining elements, causing pointer invalidation.
        // For each domain, clear its static memory slice for this timestamp index
        for (int domain_i = 0; domain_i < EXTLIB_NUM_DOMAINS; domain_i++)
        {
            size_t arr_size = timestamp->domain_data[domain_i].arr_size;
            if (arr_size == 0)
                continue;
            size_t offset = arr_size * timestamp->static_memory_index;

            // just clear the values.
            if (OPT_LIKELY(timestamp->domain_data[domain_i].counter_value))
            {
                memset(&timestamp->domain_data[domain_i].counter_value, 0, arr_size * sizeof(uint64_t));
            }
            if (OPT_LIKELY(timestamp->domain_data[domain_i].energy_per_counter))
            {
                memset(&timestamp->domain_data[domain_i].energy_per_counter, 0, arr_size * sizeof(uint64_t));
            }
        }

        // Clear the ExtlibEnergyTimeStamp struct itself
        memset(timestamp, 0, sizeof(struct ExtlibEnergyTimeStamp));
        timestamp->id = 0; // Mark as available
        timestamp->static_memory_index = -1;
    }
}
struct ExtlibEnergyTimeStamp *extlib_calc_energy_consumption(struct ExtlibEnergyTimeStamp *begin, struct ExtlibEnergyTimeStamp *end)
{
    if (OPT_UNLIKELY(begin == NULL))
    {
        perror("[ERROR]: Begin timestamp is NULL\n");
        return NULL;
    }
    if (OPT_UNLIKELY(end == NULL))
    {
        perror("[ERROR]: End timestamp is NULL\n");
        return NULL;
    }

    // Check array compatibility
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
    if (!result)
        return NULL;

    for (int i = 0; i < EXTLIB_NUM_DOMAINS; i++)
    {
        if (OPT_LIKELY(begin->domain_data[i].counter_value != NULL && begin->domain_data[i].counter_name != NULL))
        {
            result->domain_data[i].arr_size = begin->domain_data[i].arr_size;
            result->domain_data[i].domain_id = begin->domain_data[i].domain_id;
            // std::cout << "\tarr_size: " << result->domain_data[i].arr_size << " domain_id:" << (int)result->domain_data[i].domain_id << "\n";

            if (use_static_memory)
            {
                // std::cout << "\textlib_calc_energy_consumption use_static_memory\n";
                DomainMemoryMap &map = domain_pointer_map[i];
                size_t offset = map.arr_size * result->static_memory_index;
                // std::cout << "\toffset:" << offset << " -- map.arr_size:" << map.arr_size << " -- " << "static memory index:" << result->static_memory_index << "\n";
                result->domain_data[i].counter_name = map.counter_name; // this is okay, all will point to the same memory location.
                result->domain_data[i].counter_value = &map.counter_value[offset];
                result->domain_data[i].energy_per_counter = &map.energy_per_counter[offset];
            }
            else
            {
                // std::cout << "\textlib_calc_energy_consumption use_dynamic_memory\n";
                result->domain_data[i].energy_per_counter = (double *)malloc(begin->domain_data[i].arr_size * sizeof(double));
                if (OPT_UNLIKELY(result->domain_data[i].energy_per_counter == NULL))
                {
                    perror("[ERROR]: Failed to allocate energy_per_counter");
                    extlib_free_energy_timestamp(result);
                    return NULL;
                }

                result->domain_data[i].counter_name = (char **)malloc(begin->domain_data[i].arr_size * sizeof(char *));
                if (OPT_UNLIKELY(result->domain_data[i].counter_name == NULL))
                {
                    perror("[ERROR]: Failed to allocate counter_name");
                    extlib_free_energy_timestamp(result);
                    return NULL;
                }

                for (size_t j = 0; j < begin->domain_data[i].arr_size; j++)
                {
                    result->domain_data[i].counter_name[j] = strdup(begin->domain_data[i].counter_name[j]);
                    if (!result->domain_data[i].counter_name[j])
                    {
                        fprintf(stderr, "[ERROR]: Failed strdup for counter_name[%zu]\n", j);
                        extlib_free_energy_timestamp(result);
                        return NULL;
                    }
                }
            }

            // Compute energy values
            int cumulative = 0;
            for (size_t j = 0; j < begin->domain_data[i].arr_size; j++)
            {
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
                    break;
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