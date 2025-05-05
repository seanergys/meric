#pragma once

#ifndef SRC_WRAPPER_RAPLWRAPPER_H_
#define SRC_WRAPPER_RAPLWRAPPER_H_

#include <string>
#include <map>
#include <memory>

#include "msrs.h" // FIXME: Propagates "msr_whitelist" into "systemInfo.cpp"

namespace meric {

class RAPL
{
public:
    class Base {
    public:
        typedef unsigned long long int RawCounterValue_t;
        typedef std::map<std::string, RawCounterValue_t> CounterRecord_t;

        virtual void init(bool detailed) = 0;
        virtual void close() = 0;

        virtual void getCounterValues(CounterRecord_t &record) = 0;
        virtual double getResultValue(RawCounterValue_t startValue, RawCounterValue_t stopValue, 
            double runtime, int &cumulative, std::string counter, std::string regionName = "") = 0;
    
        virtual std::string summaryCounter() = 0;
        virtual std::string getSummaryCounterName() = 0;
        virtual std::string getSourceLabel() = 0;
        virtual std::string getCounterLabel(const std::string counter) = 0;
    
        virtual void getRatios(CounterRecord_t &start, CounterRecord_t &stop,
            double runtime, std::map<std::string, double> &ratios) = 0;
    
        virtual void initCountersUnits() = 0;
        virtual double getUnit(std::string counter) = 0;

        static bool isAvailable() { return false; }
    };

    typedef Base::RawCounterValue_t RawCounterValue_t;
    typedef Base::CounterRecord_t CounterRecord_t;

    static void init(bool detailed);
    static void close();

    static void getCounterValues(CounterRecord_t &record);
    static double getResultValue(RawCounterValue_t startValue, RawCounterValue_t stopValue, 
        double runtime, int &cumulative, std::string counter, std::string regionName = "");

    static std::string summaryCounter();
    static std::string getSummaryCounterName();
    static std::string getSourceLabel();
    static std::string getCounterLabel(const std::string counter);

    static void getRatios(CounterRecord_t &start, CounterRecord_t &stop,
        double runtime, std::map<std::string, double> &ratios);

    static void initCountersUnits();
    static double getUnit(std::string counter);

    static std::string PCKG_POWER_LIMIT;
    static std::string RAM_POWER_LIMIT;
    static std::string PLATFORM_POWER_LIMIT;
    static std::string POWER_LIMIT_TIMEWINDOW;

protected:
    static Base *getImpl();
};

}

#endif /* SRC_WRAPPER_RAPLWRAPPER_H_ */