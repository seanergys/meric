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
struct DomainMemoryMap                    //        ---------- PAGE 0 ----------                ----------- PAGE 1 ----------       ....
{                                         //    index1  index2   index3   index4   index5         index1  index2   index3   index4   index5
    char **counter_name = nullptr;        // [--------]
    uint64_t *counter_value = nullptr;    // [--------|--------|--------|--------|--------...] [--------|--------|--------|--------|--------...]
    double *energy_per_counter = nullptr; // [--------|--------|--------|--------|--------...] [--------|--------|--------|--------|--------...]
    size_t arr_size = 0;
};
static std::unordered_map<int, std::unordered_map<int, DomainMemoryMap>> energyts_domain_data; // domain_id - <page - memory_map>
static std::unordered_map<int, std::vector<ExtlibEnergyTimeStamp>> energyts_metadata;          // page - ExtlibEnergyTimeStamp metadata
static std::unordered_map<int, int> energyts_metadata_entry_length;                            // page - num of entries in that page
static size_t reserved_memory_size;                                                            // in num of elements
static bool use_reserved_memory = false;

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

    if (use_reserved_memory)
    {
        // TODO: get cell of domain_index since it is an array now.
        std::cout << "\tMap is recieved at page: " << page << " domain_index: " << domain_index << "\n";
        DomainMemoryMap &map = energyts_domain_data.at(domain_index).at(page);
        size_t arr_size = map.arr_size;

        // Safety check
        if (*size > arr_size)
        {
            fprintf(stderr, "[WARN]: Measurement size (%zu) exceeds allocated arr_size (%zu) for domain %d\n", *size, arr_size, domain_index);
            exit(EXIT_FAILURE);
        }

        size_t offset = reserved_memory_index * arr_size;
        // std::cout << "offset is ->" << offset << "\n";

        // get empty space adresses
        *values = &map.counter_value[offset];
        *labels = map.counter_name;

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
    // It will perform a read as normal (using dynamic memory with malloc) to see how many fields will return
    // based on the size of the fields, we allocate a chunk of memory based on the user size request to be used.
    if (enabled_domains->use_reserved_memory)
    {
        // make a read to see domain array sizes.
        struct ExtlibEnergyTimeStamp *temp = extlib_read_energy_measurements(enabled_domains);

        int page = 0; // add to the first page
        reserved_memory_size = enabled_domains->reserved_memory_size;
        energyts_metadata[page].reserve(reserved_memory_size); // reserve memory for timestamp metadata
        printf("[INFO]: Using reserved memory for %zu elements. [OK]\n", energyts_metadata[page].capacity());
        energyts_metadata_entry_length[page] = 0;

        // traverse all energy domains and create DomainMemoryMap if domain is enabled
        // then allocate reserved size for each
        for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
        {
            if (!EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain)) // Check if the domain is enabled
                continue;                                            // Skip unused domains

            int domain_index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain

            DomainMemoryMap map;
            map.arr_size = temp->domain_data[domain_index].arr_size;
            size_t total_elements = map.arr_size * reserved_memory_size; // this will store whole records.

            // Allocate flat arrays for all timestamps
            map.counter_value = (uint64_t *)calloc(total_elements, sizeof(uint64_t));  // allocate space for each sample
            map.energy_per_counter = (double *)calloc(total_elements, sizeof(double)); // allocate space for each sample.
            map.counter_name = (char **)calloc(map.arr_size, sizeof(char *));          // allocate space for ONE sample, since all samples will be the same.

            if (!map.counter_value || !map.energy_per_counter || !map.counter_name)
            {
                perror("[ERROR]: Failed to allocate reserved memory for domain arrays");
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
            energyts_domain_data[domain_index][page] = std::move(map);
        }

        extlib_free_energy_timestamp(temp);

        // IMPORTANT: DO NOT CHANGE THIS!
        // I set this at the end, since I need a normal extlib_read_energy_measurements() read at the beginning (no static_map available at that time.) I first need the numbers.
        use_reserved_memory = true;
        // std::cout << "extlib_init set use_reserved_memory:" << use_reserved_memory << "\n";
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
    static bool cntr_names_cleared = false;
    std::cout << "\tclosing sequence\n";
    for (unsigned int domain = 1; domain < ExtlibEnergy::EXTLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (EXTLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain))
        {
            int domain_id = meric::mutils::countTrailingZeros(domain);
            if (use_reserved_memory && energyts_domain_data.count(domain_id))
            {
                // TODO: Keep it as is but traverse (since it is a list now.)
                std::unordered_map<int, DomainMemoryMap> &dm_map = energyts_domain_data[domain_id];

                for (auto &pair : dm_map)
                {
                    const int page = pair.first;
                    DomainMemoryMap &dm = pair.second;

                    // counter_name must be deleted only once since it is shared across all timestamps.
                    if (!cntr_names_cleared && dm.counter_name)
                    {
                        for (size_t i = 0; i < dm.arr_size; ++i)
                            if (dm.counter_name[i])
                                free(dm.counter_name[i]);
                        free(dm.counter_name);
                        dm.counter_name = nullptr;
                        cntr_names_cleared = true;
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
            }

            if (enabled_domains->closeFns[domain_id])
            {
                enabled_domains->closeFns[domain_id]();
                printf("[INFO]: Domain %s is closed [OK]\n", printDomainName(domain));
            }
        }
    }

    cntr_names_cleared = false;
    energyts_domain_data.clear();
    energyts_metadata.clear();
    use_reserved_memory = false;
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

                printf("[INFO]: Energy is read from Domain %s [OK]\n", printDomainName(timestamp->domain_data[domain_index].domain_id));
            }
            else
                printf("[INFO]: ExtlibEnergy is NOT initialised correctly! Domain %s's energyMeasurementFns is NOT set! \n", printDomainName(timestamp->domain_data[domain_index].domain_id));
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
    auto initialize_timestamp = [](ExtlibEnergyTimeStamp *ptr, size_t page_id, size_t index) -> ExtlibEnergyTimeStamp *
    {
        std::memset(ptr, 0, sizeof(ExtlibEnergyTimeStamp));

        ptr->reserved_memory_page = page_id;
        ptr->reserved_memory_index = index;

        // just init the id and domain_id, rest will be set by callee.
        for (size_t d = 0; d < EXTLIB_NUM_DOMAINS; ++d)
            ptr->domain_data[d].domain_id = (1 << d);

        struct timespec tt;
        clock_gettime(CLOCK_REALTIME, &tt);
        ptr->id = meric::mutils::timeToDouble(tt);
        return ptr;
    };

    if (use_reserved_memory)
    {
        int first_non_full_page = -1;

        // Look for cleared slots and track first non-full page
        for (auto &entry : energyts_metadata)
        {
            int page_id = entry.first;
            auto &vec = entry.second;

            // Check for cleared slots first (higher priority)
            for (size_t i = 0; i < vec.size(); ++i)
            {
                if (vec[i].id == 0)
                {
                    std::cout << "Reusing slot: page=" << page_id << ", index=" << i << "\n";
                    energyts_metadata_entry_length[page_id] += 1;
                    return initialize_timestamp(&vec[i], page_id, i);
                }
            }

            // Track first non-full page for potential expansion
            if (first_non_full_page == -1 && vec.size() < reserved_memory_size)
            {
                first_non_full_page = page_id;
                // do not break here, I prioritize using empty slots first over expansion.
            }
        }

        // If we found a non-full page, append to it
        if (first_non_full_page != -1)
        {
            std::cout << "Appending to page: " << first_non_full_page << "\n";
            auto &vec = energyts_metadata[first_non_full_page];
            vec.emplace_back();
            energyts_metadata_entry_length[first_non_full_page] += 1;
            return initialize_timestamp(&vec.back(), first_non_full_page, vec.size() - 1);
        }

        // All pages full, create a new page
        size_t new_page_id = energyts_metadata.size();
        std::cout << "Creating new page: " << new_page_id << "\n";
        auto &new_vec = energyts_metadata[new_page_id];
        new_vec.reserve(reserved_memory_size); // Reserve space for the metadata.
        new_vec.emplace_back();                // Add a new timestamp entry.
        energyts_metadata_entry_length[new_page_id] = 1;
        std::cout << "reserved_memory_size: " << reserved_memory_size << " for page " << new_page_id << "\n";

        // TODO: need to allocate memory for the new page element pointers. (like we do with init)
        // traverse energyts_domain_data variable and first page, based on the reserved_memory_size and allocations, create another one for the new page
        for (auto &&i : energyts_domain_data)
        {
            int domain_id = i.first;
            auto &domain_map = i.second;

            // Create a new DomainMemoryMap for the new page
            DomainMemoryMap map;
            map.arr_size = domain_map[0].arr_size;                       // page 0's arr_size is the same for all pages.
            size_t total_elements = map.arr_size * reserved_memory_size; // this will store whole records.
            
            std::cout << "total_elements: " << total_elements << " for domain_id: " << domain_id << "\n";
            // Allocate flat arrays for all timestamps
            map.counter_value = (uint64_t *)calloc(total_elements, sizeof(uint64_t));  // allocate space for each sample
            map.energy_per_counter = (double *)calloc(total_elements, sizeof(double)); // allocate space for each sample.
            map.counter_name = domain_map[0].counter_name;                             // use the same counter_name array as the first page.

            if (!map.counter_value || !map.energy_per_counter || !map.counter_name)
            {
                perror("[ERROR]: Failed to allocate reserved memory for domain arrays");
                exit(EXIT_FAILURE);
            }

            energyts_domain_data[domain_id][new_page_id] = std::move(map);
        }
        return initialize_timestamp(&new_vec.back(), new_page_id, 0);
    }
    else
    {
        // Dynamic allocation
        auto *ptr = static_cast<ExtlibEnergyTimeStamp *>(malloc(sizeof(ExtlibEnergyTimeStamp)));
        if (OPT_UNLIKELY(!ptr))
        {
            perror("[ERROR]: Failed to allocate memory for EnergyTimeStamp");
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
        perror("[ERROR]:  NULL timestamp\n");
        return;
    }
    // Print pointer sliceesses and array sizes
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

    if (!use_reserved_memory)
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
        for (int domain_id = 0; domain_id < EXTLIB_NUM_DOMAINS; domain_id++)
        {
            size_t arr_size = timestamp->domain_data[domain_id].arr_size;
            if (arr_size == 0)
                continue;
            size_t offset = arr_size * timestamp->reserved_memory_index;

            // just clear the counter and energy values.
            // do not clear the name array, since it is shared across all timestamps.
            if (OPT_LIKELY(timestamp->domain_data[domain_id].counter_value))
            {
                memset(timestamp->domain_data[domain_id].counter_value, 0, arr_size * sizeof(uint64_t));
            }
            if (OPT_LIKELY(timestamp->domain_data[domain_id].energy_per_counter))
            {
                memset(timestamp->domain_data[domain_id].energy_per_counter, 0, arr_size * sizeof(uint64_t));
            }
        }

        // Remove the timestamp from the metadata vector
        // if it becomes empty, remove the page from the metadata memory map.
        auto &vec = energyts_metadata[timestamp->reserved_memory_page];
        std::cout << "\t length of page before removal: " << energyts_metadata_entry_length[timestamp->reserved_memory_page] << "\n";

        // if page-vector is empty when entry removed, delete the page.
        if (timestamp->reserved_memory_page != 0 && (energyts_metadata_entry_length[timestamp->reserved_memory_page] == 1)) // if it is the last entry in the page
        {
            energyts_metadata_entry_length[timestamp->reserved_memory_page] -= 1; // Decrease the length of the page vector

            energyts_metadata.erase(timestamp->reserved_memory_page);
            // free the memory locations pointing from the metadata
            for (int domain_id = 0; domain_id < EXTLIB_NUM_DOMAINS; domain_id++)
            {
                if(energyts_domain_data.count(domain_id) == 0)
                    continue; // skip if no data for this domain

                auto &mmap = energyts_domain_data[domain_id][timestamp->reserved_memory_page];
                if (mmap.counter_value)
                {
                    free(mmap.counter_value);
                    mmap.counter_value = nullptr;
                }
                if (mmap.energy_per_counter)
                {
                    free(mmap.energy_per_counter);
                    mmap.energy_per_counter = nullptr;
                }
                // don't free counter_name, since it is shared across all timestamps.
                std::cout << "\t Domain " << printDomainName(timestamp->domain_data[domain_id].domain_id) << " erased from page: " << timestamp->reserved_memory_page << "\n";
                energyts_domain_data[domain_id].erase(timestamp->reserved_memory_page); // Remove the page from the domain data map
            }
        }
        else
        {
            energyts_metadata_entry_length[timestamp->reserved_memory_page] -= 1; // Decrease the length of the page vector

            // Clear the ExtlibEnergyTimeStamp struct itself
            std::cout << "Clear timestamp at index " << timestamp->reserved_memory_index << " from page " << timestamp->reserved_memory_page << "\n";
            memset(timestamp, 0, sizeof(struct ExtlibEnergyTimeStamp));
            timestamp->id = 0; // Mark as available
            timestamp->reserved_memory_index = -1;
            timestamp->reserved_memory_page = -1; // Mark as available (max number)
        }
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

            if (use_reserved_memory)
            {
                // std::cout << "\textlib_calc_energy_consumption use_reserved_memory\n";

                if(energyts_domain_data.count(domain_iter) == 0)
                    continue; // skip if no data for this domain
                // TODO: Since it returns a vector now, get "cell of domain_iter" not just domain_iter.
                DomainMemoryMap &map = energyts_domain_data[domain_iter][result->reserved_memory_page];
                size_t offset = map.arr_size * result->reserved_memory_index;
                // std::cout << "\toffset:" << offset << " -- map.arr_size:" << map.arr_size << " -- " << "static memory index:" << result->reserved_memory_index << "\n";
                result->domain_data[domain_iter].counter_name = map.counter_name; // this is okay, all will point to the same memory location.
                result->domain_data[domain_iter].counter_value = &map.counter_value[offset];
                result->domain_data[domain_iter].energy_per_counter = &map.energy_per_counter[offset];
            }
            else
            {
                // std::cout << "\textlib_calc_energy_consumption use_dynamic_memory\n";
                result->domain_data[domain_iter].energy_per_counter = (double *)malloc(begin->domain_data[domain_iter].arr_size * sizeof(double));
                if (OPT_UNLIKELY(result->domain_data[domain_iter].energy_per_counter == NULL))
                {
                    perror("[ERROR]: Failed to allocate energy_per_counter");
                    extlib_free_energy_timestamp(result);
                    return NULL;
                }

                result->domain_data[domain_iter].counter_name = (char **)malloc(begin->domain_data[domain_iter].arr_size * sizeof(char *));
                if (OPT_UNLIKELY(result->domain_data[domain_iter].counter_name == NULL))
                {
                    perror("[ERROR]: Failed to allocate counter_name");
                    extlib_free_energy_timestamp(result);
                    return NULL;
                }

                for (size_t j = 0; j < begin->domain_data[domain_iter].arr_size; j++)
                {
                    result->domain_data[domain_iter].counter_name[j] = strdup(begin->domain_data[domain_iter].counter_name[j]);
                    if (!result->domain_data[domain_iter].counter_name[j])
                    {
                        fprintf(stderr, "[ERROR]: Failed strdup for counter_name[%zu]\n", j);
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
                    fprintf(stderr, "[ERROR]: Unknown domain ID %d\n", begin->domain_data[domain_iter].domain_id);
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