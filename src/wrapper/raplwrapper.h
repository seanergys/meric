#pragma once

#ifndef SRC_WRAPPER_RAPLWRAPPER_H_
#define SRC_WRAPPER_RAPLWRAPPER_H_

#include <ctime>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

#ifdef HAVE_X86_ADAPT_H
    #include <x86_adapt.h>
#elif defined HAVE_MSR_CORE_H
    #include <msr_rapl.h>
#elif defined HAVE_GEOPM_H
    #include <geopm/PlatformTopo.hpp>
    #include <geopm/PlatformIO.hpp>
    #include <geopm_hash.h>
    #include <geopm_field.h>
    static geopm::PlatformIO &pio = geopm::platform_io();
#endif

#include "msrs.h" // FIXME: Propagates "msr_whitelist" into "systemInfo.cpp"
#include "../basis/async_utils.h"

namespace meric {

class RAPL
{
public:
    class Base {
    public:
        typedef unsigned long long int RawCounterValue_t;
        
        struct CounterValue_t {
            CounterValue_t(uint64_t value = 0)
                : generation(0)
                , value(value)
            {
                if((value & (uint64_t(1) << 63)) == 0)
                    return;

                generation = (value & ~(uint64_t(1) << 63)) >> 32;
                this->value      = value & 0xFFFFFFFF;
            }

            CounterValue_t(RawCounterValue_t value, uint64_t generation)
                : generation(generation)
                , value(value)
            {}

            operator uint64_t() const {
                if(generation != 0)
                    return (generation << 32) | (value & 0xFFFFFFFF) | (uint64_t(1) << 63);
                else
                    return value;
            }

            uint64_t generation;
            RawCounterValue_t value;
        };

        typedef std::map<std::string, CounterValue_t> CounterRecord_t;

        struct CounterDesc_t {
            CounterDesc_t()
                : unit(1.0)
                , rangeMax(0)
                , isCumulative(false)
            {}

            CounterDesc_t(const CounterDesc_t &other)
                : unit(other.unit)
                , label(other.label)
                , fullName(other.fullName)
                , rangeMax(other.rangeMax)
                , isCumulative(other.isCumulative)
            {}

            double unit;
            std::string label;
            std::string fullName;
            uint64_t rangeMax;
            bool isCumulative;
        };

        virtual void init(bool detailed) = 0;
        virtual void close() = 0;

        virtual void getCounterValues(CounterRecord_t &record) = 0; // NOTE: This method is obsolete RAPL class uses "getCounterValue" instead.
        virtual double getResultValue(CounterValue_t startValue, CounterValue_t stopValue, 
            double runtime, int &cumulative, std::string counter, std::string regionName = "") = 0;
    
        virtual std::string summaryCounter() = 0;
        virtual std::string getSummaryCounterName() = 0;
        virtual std::string getSourceLabel() = 0;
        virtual std::string getCounterLabel(const std::string counter) = 0;
    
        virtual void getRatios(CounterRecord_t &start, CounterRecord_t &stop,
            double runtime, std::map<std::string, double> &ratios) = 0;
    
        virtual void initCountersUnits() = 0;
        virtual double getUnit(std::string counter) = 0;

        virtual void getCounters(std::vector<CounterDesc_t *> &counters) {}
        virtual RawCounterValue_t getCounterValue(CounterDesc_t *pCounterDesc) { return 0; }

        static bool isAvailable() { return false; }
    };

    typedef Base::RawCounterValue_t RawCounterValue_t;
    typedef Base::CounterValue_t    CounterValue_t;
    typedef Base::CounterRecord_t   CounterRecord_t;

    // Counters interface
    static void init(bool detailed);
    static void close();

    static void getCounterValues(CounterRecord_t &record);
    static double getResultValue(CounterValue_t startValue, CounterValue_t stopValue, 
        double runtime, int &cumulative, std::string counter, std::string regionName = "");

    static std::string summaryCounter();
    static std::string getSummaryCounterName();
    static std::string getSourceLabel();
    static std::string getCounterLabel(const std::string counter);

    static void getRatios(CounterRecord_t &start, CounterRecord_t &stop,
        double runtime, std::map<std::string, double> &ratios);

    static void initCountersUnits();
    static double getUnit(std::string counter);

    // Sampling interface
    static const std::string SUM_ENERGY_LABEL;
    static int PROBE_NUMBER;
    static std::vector<std::string> vrSensorLabel;

    static void start(timespec &time, double sampleFreqHz = 0.0);
    static void stop(timespec &time);
    static void initSamplers();
    static void closeSamplers();
    static int getSamples();
    static void getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> &samples);
    static std::string getData(double startTime, double duration,
        size_t *firstBlade, size_t *lastBlade, size_t *firstVR, size_t *lastVR,
        double energy[], double minWatts[], double maxWatts[]);
    static void setTimeAdjustment(timespec &time) {}
    static void fillTime(timespec &time) { clock_gettime(CLOCK_REALTIME, &time); }
    static void deleteSamples() {}

    static bool isAvailable();

    // RAPL Properties
    static std::string PCKG_POWER_LIMIT;
    static std::string RAM_POWER_LIMIT;
    static std::string PLATFORM_POWER_LIMIT;
    static std::string POWER_LIMIT_TIMEWINDOW;

protected:
    typedef std::chrono::steady_clock Clock_t;

    struct Sample_t {
        uint64_t timestamp;
        uint64_t value;
    };

    struct SamplerDesc_t {
        Base::CounterDesc_t *pCounterDesc;
        uint64_t lastValue;
        uint64_t overflowCount;
        std::vector<Sample_t> samples;
    };

    static Base *getImpl();

    static std::map<std::string, SamplerDesc_t> s_samplerDesc;
    static std::vector<SamplerDesc_t *>         s_samplerById;

    static std::chrono::nanoseconds      s_samplingPeriod;
    static std::chrono::nanoseconds      s_overflowCheckPeriod;
    static std::atomic<bool>             s_sampleStoreEnabled;
    static AsyncSampler<CounterRecord_t> s_asyncSampler;
};

}

#endif /* SRC_WRAPPER_RAPLWRAPPER_H_ */
