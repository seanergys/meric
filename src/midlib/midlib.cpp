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

#define POWER_BASELINE 0
static std::vector<std::map<std::string, unsigned long long int>> recordEnergy;
static std::vector<double> recordTime;

template <typename Tmode>
static uint64_t *measureEnergy(size_t *size, double timeNow = 0.0)
{
    if (recordEnergy.empty())
    {
        printf("Initializing energy counters...\n");
        Tmode::init(true);
        printf("Initializing energy counters [OK]\n");
    }

    if (timeNow == 0.0)
    {
        struct timespec timestamp;
        clock_gettime(CLOCK_REALTIME, &timestamp);
        timeNow = meric::mutils::timeToDouble(timestamp);
    }
    std::map<std::string, unsigned long long int> energyNow;
    Tmode::getCounterValues(energyNow);
    recordEnergy.push_back(energyNow);
    recordTime.push_back(timeNow);

    for (const auto &entry : energyNow)
    {
        std::cout << entry.first << " --> " << entry.second << "\n";
    }

    *size = energyNow.size();
    uint64_t *result = (uint64_t *)malloc(energyNow.size() * sizeof(uint64_t));
    int i = 0;
    for (const auto &entry : energyNow)
    {
        result[i] = entry.second; // Store the value (unsigned long long)
        i++;
    }
    return result;
}

template uint64_t *measureEnergy<meric::A64FX>(size_t *, double);
template uint64_t *measureEnergy<meric::RAPL>(size_t *, double);
template uint64_t *measureEnergy<meric::NVML>(size_t *, double);
// template uint64_t* measureEnergy<meric::ROCM>(size_t*, double);
// template uint64_t* measureEnergy<meric::HDEEM>(size_t*, double);

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

// template <typename Tmode>
// static double getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, int *cumulative, const char *c_counter)
// {
//     std::string counter{c_counter};
//     return Tmode::getResultValue(startValue, stopValue, runtime, *cumulative, counter);
// }
// template double getResultValue<meric::A64FX>(unsigned long long startValue, unsigned long long stopValue, double runtime, int *cumulative, const char *c_counter);
// template double getResultValue<meric::RAPL>(unsigned long long startValue, unsigned long long stopValue, double runtime, int *cumulative, const char *c_counter);
// template double getResultValue<meric::NVML>(unsigned long long startValue, unsigned long long stopValue, double runtime, int *cumulative, const char *c_counter);
// template double getResultValue<meric::ROCM>(unsigned long long startValue, unsigned long long stopValue, double runtime, int *cumulative, const char *c_counter);
// template double getResultValue<meric::HDEEM>(unsigned long long startValue, unsigned long long stopValue, double runtime, int *cumulative, const char *c_counter);

template <typename Tmode>
static void printValues(bool verbose)
{
    if (recordTime.size() < 2)
    {
        std::cerr << "ERROR missing samples\n";
        exit(2);
    }

    std::map<std::string, double> record;
    for (std::map<std::string, unsigned long long int>::iterator iterator = recordEnergy[0].begin(); iterator != recordEnergy[0].end(); iterator++)
    {
        record[iterator->first] = 0;
    }

    double summary = 0.0;
    double runtime = recordTime.back() - recordTime[0];

    for (int i = 1; i < recordEnergy.size(); i++)
    {
        for (auto iterator = record.begin(); iterator != record.end(); iterator++)
        {
            int cumulative = 0;
            record[iterator->first] += Tmode::getResultValue(recordEnergy[i - 1][iterator->first], recordEnergy[i][iterator->first], recordTime[i - 1] - recordTime[i], cumulative, iterator->first);

            std::cout << " >> " << recordEnergy[i - 1][iterator->first] << " - " << recordEnergy[i][iterator->first] << std::endl;

            if (cumulative > 0)
            {
                summary += record[iterator->first];
            }
        }
    }
    summary += POWER_BASELINE * runtime;

    if (verbose)
    {
        std::cout << "Runtime [s] = " << runtime << std::endl;

        for (std::map<std::string, unsigned long long int>::iterator iterator = recordEnergy[0].begin(); iterator != recordEnergy[0].end(); iterator++)
        {
            std::cout << std::fixed << std::setprecision(3) << Tmode::getCounterLabel(iterator->first) << " = " << record[iterator->first] << std::endl;
        }
        if (Tmode::getSummaryCounterName() != "")
        {
            std::cout << std::fixed << std::setprecision(3) << std::endl
                      << Tmode::getSummaryCounterName() << " = " << summary << std::endl;
        }
    }
    else
    {
        std::cout << std::fixed << std::setprecision(3) << summary << std::endl;
    }
}
template void printValues<meric::A64FX>(bool);
template void printValues<meric::RAPL>(bool);
template void printValues<meric::NVML>(bool);
// template void printValues<meric::ROCM>(bool);
// template void printValues<meric::HDEEM>(bool);

template <typename Tmode>
static char *getCounterLabel(const char *counter)
{
    std::string str_counter{counter};
    std::string result = Tmode::getCounterLabel(str_counter);
    char *c_str = new char[result.size() + 1]; // +1 for null terminator
    std::copy(result.begin(), result.end(), c_str);
    c_str[result.size()] = '\0'; // Null-terminate the string
    return c_str;
}
template char *getCounterLabel<meric::A64FX>(const char *counter);
template char *getCounterLabel<meric::RAPL>(const char *counter);
template char *getCounterLabel<meric::NVML>(const char *counter);
// template std::string getCounterLabel<meric::ROCM>(const char* counter);
// template std::string getCounterLabel<meric::HDEEM>(const char* counter);

void midlib_init(MidlibEnergy *enabled_domains, bool is_detailed)
{
    if (OPT_UNLIKELY(MIDLIB_ENERGY_NO_SELECTION(*enabled_domains)))
    {
        perror("Midlib No Domain Selected!\n");
        return;
    }

    meric::Environment::initHWSizes();

    for (unsigned int domain = 1; domain < MidlibEnergy::MIDLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        int index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = NULL;
        enabled_domains->printMeasurementFns[index] = NULL;
        enabled_domains->closeFns[index] = NULL;
        enabled_domains->counterLabelFNs[index] = NULL;
        // enabled_domains->getResultValueFNs[index] = NULL;
    }

    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_A64FX)))
    {
        int index = meric::mutils::countTrailingZeros(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_A64FX); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::A64FX>;
        enabled_domains->printMeasurementFns[index] = printValues<meric::A64FX>;
        enabled_domains->closeFns[index] = close<meric::A64FX>;
        enabled_domains->counterLabelFNs[index] = getCounterLabel<meric::A64FX>;
        // enabled_domains->getResultValueFNs[index] = getResultValue<meric::A64FX>;
    }
    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_RAPL)))
    {
        int index = meric::mutils::countTrailingZeros(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_RAPL); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::RAPL>;
        enabled_domains->printMeasurementFns[index] = printValues<meric::RAPL>;
        enabled_domains->closeFns[index] = close<meric::RAPL>;
        enabled_domains->counterLabelFNs[index] = getCounterLabel<meric::RAPL>;
        // enabled_domains->getResultValueFNs[index] = getResultValue<meric::RAPL>;
    }
    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_NVML)))
    {
        int index = meric::mutils::countTrailingZeros(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_NVML); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::NVML>;
        enabled_domains->printMeasurementFns[index] = printValues<meric::NVML>;
        enabled_domains->closeFns[index] = close<meric::NVML>;
        enabled_domains->counterLabelFNs[index] = getCounterLabel<meric::NVML>;
        // enabled_domains->getResultValueFNs[index] = getResultValue<meric::NVML>;
    }
    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_ROCM)))
    {
        perror("ROCM Has not yet been implemented!\n");
        // int index = meric::mutils::countTrailingZeros(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_ROCM); // Get index of the enabled domain
        // enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::ROCM>;
        // enabled_domains->printMeasurementFns[index] = printValues<meric::ROCM>;
        // enabled_domains->closeFns[index] = close<meric::ROCM>;
        // enabled_domains->counterLabelFNs[index] = getCounterLabel<meric::ROCM>;
        // enabled_domains->getResultValueFNs[index] = getResultValue<meric::ROCM>;
    }
    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergy::MIDLIB_ENERGY_DOMAIN_HDEEM)))
    {
        perror("HDEEM Has not yet been implemented!\n");
        // int index = meric::mutils::countTrailingZeros(MidlibEnergy::MIDLIB_ENERGY_DOMAIN_HDEEM); // Get index of the enabled domain
        // enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::HDEEM>;
        // enabled_domains->printMeasurementFns[index] = printValues<meric::HDEEM>;
        // enabled_domains->closeFns[index] = close<meric::HDEEM>;
        // enabled_domains->counterLabelFNs[index] = getCounterLabel<meric::HDEEM>;
        // enabled_domains->getResultValueFNs[index] = getResultValue<meric::HDEEM>;
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

char *midlib_get_counter_label(MidlibEnergy *enabled_domains, const char *counter)
{
    std::string buffer = "";

    for (unsigned int domain = 1; domain < MidlibEnergy::MIDLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain)) // Check if the domain is enabled
        {
            int index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
            if (enabled_domains->counterLabelFNs[index])
            {
                char *result = enabled_domains->counterLabelFNs[index](counter);

                // Append the result to the buffer
                buffer += result;
                buffer += ", ";

                // Free the dynamically allocated memory
                delete[] result;
            }
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
    struct MidlibEnergyTimeStamp *timestamp = (struct MidlibEnergyTimeStamp *)malloc(sizeof(struct MidlibEnergyTimeStamp));
    memset(timestamp, 0, sizeof(struct MidlibEnergyTimeStamp)); // Correct: clears struct fields
    struct timespec tt;
    clock_gettime(CLOCK_REALTIME, &tt);
    timestamp->id = meric::mutils::timeToDouble(tt);

    for (unsigned int domain = 1; domain < MidlibEnergy::MIDLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain)) // Check if the domain is enabled
        {
            int index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
            if (enabled_domains->energyMeasurementFns[index])
            {
                size_t arr_size;
                uint64_t *energy_value = enabled_domains->energyMeasurementFns[index](&arr_size, 0.0);
                switch (domain)
                {
                case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_A64FX:
                    timestamp->a64fx = energy_value;
                    timestamp->a64fx_arr_size = arr_size;
                    printf("size is:%d\n", arr_size);
                    break;
                case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_RAPL:
                    timestamp->rapl = energy_value;
                    timestamp->rapl_arr_size = arr_size;
                    break;
                case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_NVML:
                    timestamp->nvml = energy_value;
                    timestamp->nvml_arr_size = arr_size;
                    break;
                case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_ROCM:
                    timestamp->rocm = energy_value;
                    timestamp->rocm_arr_size = arr_size;
                    break;
                case MidlibEnergy::MIDLIB_ENERGY_DOMAIN_HDEEM:
                    timestamp->hdeem = energy_value;
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
static void print_array(const char *name, uint64_t *arr, size_t size, size_t print_limit)
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

void midlib_print_timestamp(struct MidlibEnergyTimeStamp *timestamp, size_t print_limit)
{
    if (OPT_UNLIKELY(timestamp == NULL))
    {
        perror("Error: NULL timestamp\n");
        return;
    }
    // Print pointer addresses and array sizes
    printf("------- MidlibEnergyTimeStamp (id: %.4f) -------\n", timestamp->id);

    print_array("a64fx", timestamp->a64fx, timestamp->a64fx_arr_size, print_limit);
    print_array("rapl", timestamp->rapl, timestamp->rapl_arr_size, print_limit);
    print_array("nvml", timestamp->nvml, timestamp->nvml_arr_size, print_limit);
    print_array("rocm", timestamp->rocm, timestamp->rocm_arr_size, print_limit);
    print_array("hdeem", timestamp->hdeem, timestamp->hdeem_arr_size, print_limit);

    printf("-----------------------------------\n");
}

void midlib_free_timestamp(struct MidlibEnergyTimeStamp *timestamp)
{
    if (OPT_UNLIKELY(timestamp == NULL))
        return;

    if (OPT_LIKELY(timestamp->a64fx != NULL))
        free(timestamp->a64fx);
    if (OPT_LIKELY(timestamp->rapl != NULL))
        free(timestamp->rapl);
    if (OPT_LIKELY(timestamp->nvml != NULL))
        free(timestamp->nvml);
    if (OPT_LIKELY(timestamp->rocm != NULL))
        free(timestamp->rocm);
    if (OPT_LIKELY(timestamp->hdeem != NULL))
        free(timestamp->hdeem);

    memset(timestamp, 0, sizeof(struct MidlibEnergyTimeStamp)); // Correct: clears struct fields
}

void midlib_print_energy_measurements(MidlibEnergy *enabled_domains)
{
    for (unsigned int domain = 1; domain < MidlibEnergy::MIDLIB_ENERGY_DOMAIN_END; domain <<= 1)
    {
        if (MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain)) // Check if the domain is enabled
        {
            int index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
            if (enabled_domains->energyMeasurementFns[index])
            {
                enabled_domains->printMeasurementFns[index](true);
            }
        }
    }
}

// double midlib_get_result_value(MidlibEnergy *enabled_domains, unsigned long long startValue, unsigned long long stopValue, double runtime, int *cumulative, const char *counter)
// {
//     for (unsigned int domain = 1; domain < MidlibEnergy::END; domain <<= 1)
//     {
//         if (MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain)) // Check if the domain is enabled
//         {
//             int index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
//             if (enabled_domains->energyMeasurementFns[index])
//             {
//                 return enabled_domains->getResultValueFNs[index](startValue, stopValue, runtime, cumulative, counter);
//             }
//         }
//     }
// }

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
