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
#include "reserved_memory_manager.h"

/*
    --------- Compilation Unit Only Method Declerations ---------

    These methods are not present in the header file &
    may or maynot be called indirectly by the extlib_* methods

*/
struct ExtlibEnergyTimeStamp *newEnergyTimeStamp();
static const char *printDomainName(unsigned int value);
template <typename Tmode>
static uint64_t *measureEnergy(size_t *size, double timeNow = 0.0);
template <typename Tmode>
static void close();
template <typename Tmode>
static void printArray(const char *name, Tmode *arr, size_t size, size_t print_limit);

/* --------------------------------------------------------------- */

template <typename Tmode, typename TmapVal>
static void measureEnergy(size_t *size, uint64_t **values, char ***labels, size_t page, size_t reserved_memory_index, size_t domain_index)
{
    std::cout << "\tmeasureEnergy called for domain_index: " << domain_index << " page: " << page << "\n";
    std::map<std::string, TmapVal> energyNow;
    Tmode::getCounterValues(energyNow);
    *size = energyNow.size();
    std::cout << "" << "\tsize of energyNow: " << *size << "\n";

    if (ReservedMemoryManager::is_enabled)
    {
        std::cout << "\tMap is recieved at page: " << page << " domain_index: " << domain_index << "\n";
        ReservedMemoryManager::DomainMemoryPool *map = ReservedMemoryManager::get_domain_map(domain_index, page);
        if (map == nullptr)
        {
            std::cerr << "[ERROR]: Failed to get domain map for domain " << domain_index << " page " << page << "\n";
            // exit(EXIT_FAILURE);
            return;
        }
        size_t arr_size = map->arr_size;

        // Safety check
        if (*size > arr_size)
        {
            std::cerr << "[WARN]: Measurement size (" << *size << ") exceeds allocated arr_size (" << arr_size << ") for domain " << domain_index << std::endl;
            exit(EXIT_FAILURE);
        }

        size_t offset = reserved_memory_index * arr_size;
        // std::cout << "offset is ->" << offset << "\n";

        // get empty space adresses
        *values = &map->counter_value[offset];
        *labels = map->counter_name;

        // fill with data, data here is the counter values.
        size_t i = 0;
        for (const auto &entry : energyNow)
        {
            if (i >= arr_size)
                break;

            (*values)[i] = static_cast<uint64_t>(entry.second);
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
            (*labels)[i] = (char *)malloc(EXTLIB_MAX_DOMAIN_NAME_LENGTH);
            if ((*labels)[i])
            {
                strncpy((*labels)[i], entry.first.c_str(), EXTLIB_MAX_DOMAIN_NAME_LENGTH - 1);
                (*labels)[i][EXTLIB_MAX_DOMAIN_NAME_LENGTH - 1] = '\0';
            }
            ++i;
        }
    }
}

// Deliberate template explosion for explicit domains
template static void measureEnergy<meric::A64FX, unsigned long long int>(size_t *, uint64_t **, char ***, size_t, size_t, size_t);
template static void measureEnergy<meric::RAPL, unsigned long long int>(size_t *, uint64_t **, char ***, size_t, size_t, size_t);
template static void measureEnergy<meric::NVML, unsigned long long int>(size_t *, uint64_t **, char ***, size_t, size_t, size_t);
template static void measureEnergy<meric::HWMON, unsigned long long int>(size_t *, uint64_t **, char ***, size_t, size_t, size_t);
template static void measureEnergy<meric::ROCM, uint64_t>(size_t *, uint64_t **, char ***, size_t, size_t, size_t);
template static void measureEnergy<meric::HDEEM, double>(size_t *, uint64_t **, char ***, size_t, size_t, size_t);

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
    std::cout << "| " << std::setw(15) << std::left << name << " (size: " << size << "): ";

    if (!arr || size == 0)
    {
        std::cout << "NULL or empty array" << std::endl;
        return;
    }

    std::cout << "[";

    print_limit = (print_limit == static_cast<size_t>(-1)) ? size : print_limit;
    for (size_t i = 0; i < size && i < print_limit; ++i)
    {
        if (std::is_same<Tmode, double>::value)
        {
            std::cout << arr[i];
        }
        else if (std::is_same<Tmode, uint64_t>::value)
        {
            std::cout << arr[i];
        }
        else if (std::is_same<Tmode, char *>::value)
        {
            std::cout << arr[i];
        }

        if (i < size - 1 && i < print_limit - 1)
        {
            std::cout << ", ";
        }
    }

    if (size > print_limit)
    {
        std::cout << ", ..."; // Indicate truncated output
    }

    std::cout << "]" << std::endl;
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
        std::cout << "[INFO]: Domain " << printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX) << " is init [OK]" << std::endl;
#else
        std::cerr << "[ERROR]: Turning off " << printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX) << ". It is selected but does NOT exists on this machine!" << std::endl;
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
        std::cout << "[INFO]: Domain " << printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL) << " is init [OK]" << std::endl;
#else
        std::cerr << "[ERROR]: Turning off " << printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL) << ". It is selected but does NOT exists on this machine!" << std::endl;
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
        std::cout << "[INFO]: Domain " << printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML) << " is init [OK]" << std::endl;
#else
        std::cerr << "[ERROR]: Turning off " << printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML) << ". It is selected but does NOT exists on this machine!" << std::endl;
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
        std::cout << "[INFO]: Domain " << printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM) << " is init [OK]" << std::endl;
#else
        std::cerr << "[ERROR]: Turning off " << printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM) << ". It is selected but does NOT exists on this machine!" << std::endl;
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
        std::cout << "[INFO]: Domain " << printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM) << " is init [OK]" << std::endl;
#else
        std::cerr << "[ERROR]: Turning off " << printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM) << ". It is selected but does NOT exists on this machine!" << std::endl;
        EXTLIB_ENERGY_DISABLE_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM);
#endif
    }

    if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON)))
    {
        meric::HWMON::init(is_detailed);
        int index = meric::mutils::countTrailingZeros(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::HWMON, unsigned long long int>;
        enabled_domains->closeFns[index] = close<meric::HWMON>;
        std::cout << "[INFO]: Domain " << printDomainName(ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON) << " is init [OK]" << std::endl;
    }

    // This executes only once.
    // It will perform a read as normal (using dynamic memory with malloc) to see how many fields will return
    // based on the size of the fields, we allocate a chunk of memory based on the user size request to be used.
    if (enabled_domains->use_reserved_memory)
    {
        // make a read to see domain array sizes.
        struct ExtlibEnergyTimeStamp *template_timestamp = extlib_read_energy_measurements(enabled_domains);

        ReservedMemoryManager::initialize(enabled_domains->reserved_memory_size, template_timestamp);

        extlib_free_energy_timestamp(template_timestamp);

        // IMPORTANT: DO NOT CHANGE THIS!
        // I set this at the end, since I need a normal extlib_read_energy_measurements() read at the beginning (no static_map available at that time.) I first need the numbers.
        ReservedMemoryManager::is_enabled = true;
        // std::cout << "extlib_init set use_reserved_memory:" << use_reserved_memory << "\n";
    }
    else
    {
        std::cout << "[INFO]: Using dynamic memory. [OK]" << std::endl;
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
    std::cout << "\tclosing sequence\n";

    for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain))
        {
            int domain_id = meric::mutils::countTrailingZeros(domain);
            if (enabled_domains->closeFns[domain_id])
            {
                enabled_domains->closeFns[domain_id]();
                std::cout << "[INFO]: Domain " << printDomainName(domain) << " is closed [OK]" << std::endl;
            }
        }
    }

    std::memset(enabled_domains, 0, sizeof(ExtlibEnergy));

    if (ReservedMemoryManager::is_enabled)
        ReservedMemoryManager::close();
}

ExtlibEnergyTimeStamp *extlib_read_energy_measurements(ExtlibEnergy *enabled_domains)
{
    struct ExtlibEnergyTimeStamp *timestamp = newEnergyTimeStamp();

    for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (OPT_LIKELY(EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain))) // Check if the domain is enabled
        {
            int domain_index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
            if (OPT_LIKELY(enabled_domains->energyMeasurementFns[domain_index]))
            {
                enabled_domains->energyMeasurementFns[domain_index](
                    &timestamp->domain_data[domain_index].arr_size,
                    &timestamp->domain_data[domain_index].counter_value,
                    &timestamp->domain_data[domain_index].counter_name,
                    timestamp->reserved_memory_page,
                    timestamp->reserved_memory_index,
                    domain_index);

                std::cout << "[INFO]: Energy is read from Domain " << printDomainName(timestamp->domain_data[domain_index].domain_id) << " [OK]" << std::endl;
            }
            else
                std::cout << "[INFO]: ExtlibEnergy is NOT initialised correctly! Domain " << printDomainName(timestamp->domain_data[domain_index].domain_id) << "'s energyMeasurementFns is NOT set!" << std::endl;
        }
    }
    return timestamp;
}

/**
 * if use_reserved_memory, try to add to the end of the vector.
 * if you reach the end, check whole vector for a free memory location. if not found, exit with error.
 */
struct ExtlibEnergyTimeStamp *newEnergyTimeStamp()
{
    if (ReservedMemoryManager::is_enabled)
    {
        return ReservedMemoryManager::allocate_timestamp();
    }
    else
    {
        // Dynamic allocation
        auto *ptr = static_cast<ExtlibEnergyTimeStamp *>(malloc(sizeof(ExtlibEnergyTimeStamp)));
        if (OPT_UNLIKELY(!ptr))
        {
            std::cerr << "[ERROR]: Failed to allocate memory for EnergyTimeStamp" << std::endl;
            exit(EXIT_FAILURE);
        }

        std::memset(ptr, 0, sizeof(ExtlibEnergyTimeStamp));
        // just init the id and domain_id, rest will be set by callee.
        for (size_t d = 0; d < EXTLIB_NUM_DOMAINS; ++d)
            ptr->domain_data[d].domain_id = (1 << d);

        struct timespec tt;
        clock_gettime(CLOCK_REALTIME, &tt);
        ptr->id = meric::mutils::timeToDouble(tt);

        return ptr;
    }
}

void extlib_print_energy_timestamp(struct ExtlibEnergyTimeStamp *timestamp, size_t print_limit)
{

    if (OPT_UNLIKELY(timestamp == NULL))
    {
        std::cerr << "[ERROR]: NULL timestamp" << std::endl;
        return;
    }
    // Print pointer sliceesses and array sizes
    std::cout << "------- ExtlibEnergyTimeStamp (id: " << std::fixed << std::setprecision(4) << timestamp->id << ") -------" << std::endl;

    for (int i = 0; i < EXTLIB_NUM_DOMAINS; i++)
    {
        char domain_name_buffer[EXTLIB_MAX_DOMAIN_NAME_LENGTH];

        const char *domain_name = printDomainName(timestamp->domain_data[i].domain_id);

        snprintf(domain_name_buffer, sizeof(domain_name_buffer), "%s_counter_name", domain_name);
        printArray(domain_name_buffer, timestamp->domain_data[i].counter_name, timestamp->domain_data[i].arr_size, print_limit);

        snprintf(domain_name_buffer, sizeof(domain_name_buffer), "%s_counter_value", domain_name);
        printArray(domain_name_buffer, timestamp->domain_data[i].counter_value, timestamp->domain_data[i].arr_size, print_limit);

        snprintf(domain_name_buffer, sizeof(domain_name_buffer), "%s_energy_per_counter", domain_name);
        printArray(domain_name_buffer, timestamp->domain_data[i].energy_per_counter, timestamp->domain_data[i].arr_size, print_limit);
        std::cout << "| " << domain_name << "_total_energy: " << timestamp->domain_data[i].energy_total << std::endl;
        std::cout << "|" << std::endl;
    }
    std::cout << "| Total_energy consumption: " << timestamp->energy_total << " [J]" << std::endl;
    std::cout << "-----------------------------------" << std::endl;
}

void extlib_free_energy_timestamp(struct ExtlibEnergyTimeStamp *timestamp)
{
    if (timestamp == NULL)
        return;

    if (!ReservedMemoryManager::is_enabled)
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
        ReservedMemoryManager::deallocate_timestamp(timestamp);
    }
}
struct ExtlibEnergyTimeStamp *extlib_calc_energy_consumption(struct ExtlibEnergyTimeStamp *begin, struct ExtlibEnergyTimeStamp *end)
{
    if (OPT_UNLIKELY(begin == NULL))
    {
        std::cerr << "[ERROR]: Begin timestamp is NULL" << std::endl;
        return NULL;
    }
    if (OPT_UNLIKELY(end == NULL))
    {
        std::cerr << "[ERROR]: End timestamp is NULL" << std::endl;
        return NULL;
    }

    // Check array compatibility
    for (int i = 0; i < EXTLIB_NUM_DOMAINS; i++)
    {
        if (OPT_UNLIKELY((begin->domain_data[i].counter_name == NULL) != (end->domain_data[i].counter_name == NULL) ||
                         (begin->domain_data[i].counter_value == NULL) != (end->domain_data[i].counter_value == NULL) ||
                         begin->domain_data[i].arr_size != end->domain_data[i].arr_size))
        {
            std::cerr << "[ERROR]: Arrays for domain " << printDomainName(begin->domain_data[i].domain_id) << " are incompatible" << std::endl;
            return NULL;
        }
    }

    struct ExtlibEnergyTimeStamp *result = newEnergyTimeStamp();
    if (!result)
        return NULL;

    for (int domain_iter = 0; domain_iter < EXTLIB_NUM_DOMAINS; domain_iter++)
    {
        if (OPT_LIKELY(begin->domain_data[domain_iter].counter_value != NULL && begin->domain_data[domain_iter].counter_name != NULL))
        {
            result->domain_data[domain_iter].arr_size = begin->domain_data[domain_iter].arr_size;
            result->domain_data[domain_iter].domain_id = begin->domain_data[domain_iter].domain_id;
            // std::cout << "\tarr_size: " << result->domain_data[domain_iter].arr_size << " domain_id:" << (int)result->domain_data[domain_iter].domain_id << "\n";

            if (ReservedMemoryManager::is_enabled)
            {
                // std::cout << "\textlib_calc_energy_consumption use_reserved_memory\n";
                ReservedMemoryManager::DomainMemoryPool *map = ReservedMemoryManager::get_domain_map(domain_iter, result->reserved_memory_page); // energyts_domain_data[domain_iter][result->reserved_memory_page];
                if (map == nullptr)
                    continue;

                size_t offset = map->arr_size * result->reserved_memory_index;
                // std::cout << "\toffset:" << offset << " -- map.arr_size:" << map.arr_size << " -- " << "static memory index:" << result->reserved_memory_index << "\n";
                result->domain_data[domain_iter].counter_name = map->counter_name; // this is okay, all will point to the same memory location.
                result->domain_data[domain_iter].counter_value = &map->counter_value[offset];
                result->domain_data[domain_iter].energy_per_counter = &map->energy_per_counter[offset];
            }
            else
            {
                // std::cout << "\textlib_calc_energy_consumption use_dynamic_memory\n";
                result->domain_data[domain_iter].energy_per_counter = (double *)malloc(begin->domain_data[domain_iter].arr_size * sizeof(double));
                if (OPT_UNLIKELY(result->domain_data[domain_iter].energy_per_counter == NULL))
                {
                    std::cerr << "[ERROR]: Failed to allocate energy_per_counter" << std::endl;
                    extlib_free_energy_timestamp(result);
                    return NULL;
                }

                result->domain_data[domain_iter].counter_name = (char **)malloc(begin->domain_data[domain_iter].arr_size * sizeof(char *));
                if (OPT_UNLIKELY(result->domain_data[domain_iter].counter_name == NULL))
                {
                    std::cerr << "[ERROR]: Failed to allocate counter_name" << std::endl;
                    extlib_free_energy_timestamp(result);
                    return NULL;
                }

                for (size_t j = 0; j < begin->domain_data[domain_iter].arr_size; j++)
                {
                    result->domain_data[domain_iter].counter_name[j] = strdup(begin->domain_data[domain_iter].counter_name[j]);
                    if (!result->domain_data[domain_iter].counter_name[j])
                    {
                        std::cerr << "[ERROR]: Failed strdup for counter_name[" << j << "]" << std::endl;
                        extlib_free_energy_timestamp(result);
                        return NULL;
                    }
                }
            }

            // Compute energy values
            int cumulative = 0;
            for (size_t j = 0; j < begin->domain_data[domain_iter].arr_size; j++)
            {
                switch (begin->domain_data[domain_iter].domain_id)
                {
                case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_A64FX:
                    cumulative = 0;
                    result->domain_data[domain_iter].energy_per_counter[j] = meric::A64FX::getResultValue(
                        begin->domain_data[domain_iter].counter_value[j], end->domain_data[domain_iter].counter_value[j],
                        (end->id - begin->id), cumulative, begin->domain_data[domain_iter].counter_name[j]);
                    break;

                case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_RAPL:
                    cumulative = 1;
                    result->domain_data[domain_iter].energy_per_counter[j] = meric::RAPL::getResultValue(
                        begin->domain_data[domain_iter].counter_value[j], end->domain_data[domain_iter].counter_value[j],
                        (end->id - begin->id), cumulative, begin->domain_data[domain_iter].counter_name[j]);
                    break;

                case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_NVML:
                    cumulative = 0;
                    result->domain_data[domain_iter].energy_per_counter[j] = meric::NVML::getResultValue(
                        begin->domain_data[domain_iter].counter_value[j], end->domain_data[domain_iter].counter_value[j],
                        (end->id - begin->id), cumulative, begin->domain_data[domain_iter].counter_name[j]);
                    break;

                case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_ROCM:
                    result->domain_data[domain_iter].energy_per_counter[j] = meric::ROCM::getResultValue(
                        begin->domain_data[domain_iter].counter_value[j], end->domain_data[domain_iter].counter_value[j],
                        (end->id - begin->id), cumulative, begin->domain_data[domain_iter].counter_name[j]);
                    break;

                case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HDEEM:
                    result->domain_data[domain_iter].energy_per_counter[j] = meric::HDEEM::getResultValue(
                        begin->domain_data[domain_iter].counter_value[j], end->domain_data[domain_iter].counter_value[j],
                        (end->id - begin->id), cumulative, begin->domain_data[domain_iter].counter_name[j], "");
                    break;

                case ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_HWMON:
                    cumulative = 0;
                    result->domain_data[domain_iter].energy_per_counter[j] = meric::HWMON::getResultValue(
                        begin->domain_data[domain_iter].counter_value[j], end->domain_data[domain_iter].counter_value[j],
                        (end->id - begin->id), cumulative, begin->domain_data[domain_iter].counter_name[j]);
                    break;

                default:
                    std::cerr << "[ERROR]: Unknown domain ID " << begin->domain_data[domain_iter].domain_id << std::endl;
                    break;
                }

                result->domain_data[domain_iter].energy_total += result->domain_data[domain_iter].energy_per_counter[j];
            }

            result->energy_total += result->domain_data[domain_iter].energy_total;
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

void extlib_shrink_to_fit()
{
    if (ReservedMemoryManager::is_enabled)
        ReservedMemoryManager::shrink_to_fit();
}