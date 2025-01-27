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
static void measureEnergy(bool verbose, double timeNow = 0.0)
{
    if (recordEnergy.empty())
    {
        printf("Initializing energy counters...\n");
        Tmode::init(true);
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
}
template void measureEnergy<meric::A64FX>(bool, double);
template void measureEnergy<meric::RAPL>(bool, double);
template void measureEnergy<meric::NVML>(bool, double);
// template void measureEnergy<meric::ROCM>(bool, double);
// template void measureEnergy<meric::HDEEM>(bool, double);



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


void midlib_init(MidlibEnergyDomains *enabled_domains, bool is_detailed)
{
    if (OPT_UNLIKELY(MIDLIB_ENERGY_NO_SELECTION(*enabled_domains)))
    {
        perror("Midlib No Domain Selected!");
        return;
    }

    meric::Environment::initHWSizes();

    for (unsigned int domain = 1; domain < MidlibEnergyDomains::END; domain <<= 1)
    {
        int index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = NULL;
        enabled_domains->printMeasurementFns[index] = NULL;
        enabled_domains->closeFns[index] = NULL;
    }

    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergyDomains::A64FX)))
    {
        int index = meric::mutils::countTrailingZeros(MidlibEnergyDomains::A64FX); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::A64FX>;
        enabled_domains->printMeasurementFns[index] = printValues<meric::A64FX>;
        enabled_domains->closeFns[index] = close<meric::A64FX>;
    }
    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergyDomains::RAPL)))
    {
        int index = meric::mutils::countTrailingZeros(MidlibEnergyDomains::RAPL); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::RAPL>;
        enabled_domains->printMeasurementFns[index] = printValues<meric::RAPL>;
        enabled_domains->closeFns[index] = close<meric::RAPL>;
    }
    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergyDomains::NVML)))
    {
        int index = meric::mutils::countTrailingZeros(MidlibEnergyDomains::NVML); // Get index of the enabled domain
        enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::NVML>;
        enabled_domains->printMeasurementFns[index] = printValues<meric::NVML>;
        enabled_domains->closeFns[index] = close<meric::NVML>;
    }
    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergyDomains::ROCM)))
    {
        perror("ROCM Has not yet been implemented\n!");        
        // int index = meric::mutils::countTrailingZeros(MidlibEnergyDomains::ROCM); // Get index of the enabled domain
        // enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::ROCM>;
        // enabled_domains->printMeasurementFns[index] = printValues<meric::ROCM>;
        // enabled_domains->closeFns[index] = close<meric::ROCM>;
    }
    if (OPT_LIKELY(MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, MidlibEnergyDomains::HDEEM)))
    {
        perror("HDEEM Has not yet been implemented\n!");        
        // int index = meric::mutils::countTrailingZeros(MidlibEnergyDomains::HDEEM); // Get index of the enabled domain
        // enabled_domains->energyMeasurementFns[index] = measureEnergy<meric::HDEEM>;
        // enabled_domains->printMeasurementFns[index] = printValues<meric::HDEEM>;
        // enabled_domains->closeFns[index] = close<meric::HDEEM>;
    }
} 

void midlib_close(MidlibEnergyDomains *enabled_domains)
{
    for (unsigned int domain = 1; domain < MidlibEnergyDomains::END; domain <<= 1)
    {
        if (MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain)) // Check if the domain is enabled
        {
            int index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
            if (enabled_domains->closeFns[index])
            {
                enabled_domains->closeFns[index]();
                printf("Domain %s is closed.\n", midlib_printDomainName(domain));
            }
        }
    }
}

void midlib_read_energy_measurements(MidlibEnergyDomains *enabled_domains)
{
    for (unsigned int domain = 1; domain < MidlibEnergyDomains::END; domain <<= 1)
    {
        if (MIDLIB_ENERGY_HAS_DOMAIN(*enabled_domains, domain)) // Check if the domain is enabled
        {
            printf("Domain %s is enabled.\n", midlib_printDomainName(domain));

            int index = meric::mutils::countTrailingZeros(domain); // Get index of the enabled domain
            if (enabled_domains->energyMeasurementFns[index])
            {
                enabled_domains->energyMeasurementFns[index](true, 0.0);
            }
        }
    }
}

void midlib_print_energy_measurements(MidlibEnergyDomains *enabled_domains)
{
    for (unsigned int domain = 1; domain < MidlibEnergyDomains::END; domain <<= 1)
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
// Function to print the corresponding enum name based on the value
const char* midlib_printDomainName(unsigned int value)
{
    switch(value)
    {
        case MidlibEnergyDomains::A64FX:
            return "A64FX";
            break;
        case MidlibEnergyDomains::RAPL:
            return "RAPL";
            break;
        case MidlibEnergyDomains::NVML:
            return "NVML";
            break;
        case MidlibEnergyDomains::ROCM:
            return "ROCM";
            break;
        case MidlibEnergyDomains::HDEEM:
            return "HDEEM";
            break;
        case MidlibEnergyDomains::END:
            return "END";
            break;
        case MidlibEnergyDomains::NO_SELECTION:
            return "NO_SELECTION";
            break;
        default:
            return "Unknown Domain";
            break;
    }
}
