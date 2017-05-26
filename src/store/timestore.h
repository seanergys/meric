
#ifndef SRC_BASIS_TIMESTORE_H_
#define SRC_BASIS_TIMESTORE_H_

#include "store.h"
#include <unistd.h>

namespace meric {

struct TimeRecord {
	TimeRecord(int region, timespec &time)
	: time(time), region(region) {};

	timespec time;
	int region;
};

template <class TStore>
class AggregateStore;

template <class TStore>
class DetailedAggregateStore;

class TimeStore: public Store {

	enum WRAPPERS {HDEEM, JETSON};

	friend class AggregateStore<TimeStore>;
	friend class DetailedAggregateStore<TimeStore>;
public:
	virtual void addStart(int region);
	virtual void addEnd(int region);

	void fillRegionStats(RegionStats &stats, size_t iStarts, size_t iEnd);
	void fillRegionStats(DetailedRegionStats &stats, size_t iStarts, size_t iEnd);

	virtual void save(const std::string outputDir, const std::string outputFilename, std::vector<std::string> regionNames, const std::string prefix = "")
	{
		_name = regionNames;
		save(prefix, outputDir, outputFilename, [&] (std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) {
			saveInterval(ss, iStarts, iEnd);
		});
	}

	virtual void save(const std::string prefix, const std::string outputDir, const std::string outputFilename, 
                      std::function<void(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		sleep(2); // wait for HDEEM to load all samples
		computePeriods();
		timespec time1, time2;
		
		clock_gettime(CLOCK_REALTIME, &time1);
		if (wrapperType == HDEEM)
		{
			HDEEM::getSamples(_samples);
			if(_samples.nb_blade_values == 0)
			{
				HDEEM::deleteSamples(_samples);
				std::cerr << "MERIC ERROR: No HDEEM samples\n";
				throw 1;
			}
		}
		else if (wrapperType == JETSON)
			JETSON::getSamples(_samplesJetson);
		clock_gettime(CLOCK_REALTIME, &time2);
		MERIC_INFO << "MERIC GET SAMPLES: " << mutils::timeToDouble(time2 - time1) <<" s\n";
		
		iterate(_records, prefix, outputDir, outputFilename, processInterval);

		if (wrapperType == HDEEM)
		{
			clock_gettime(CLOCK_REALTIME, &time1);
			HDEEM::deleteSamples(_samples);
			clock_gettime(CLOCK_REALTIME, &time2);
			MERIC_INFO << "MERIC DELETE HDEEM SAMPLES: " << mutils::timeToDouble(time2 - time1)<<" s\n";
		}
	}
	virtual void save(std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		sleep(2); // wait for HDEEM to load all samples
		computePeriods();
		timespec time1, time2;
	
		clock_gettime(CLOCK_REALTIME, &time1);

		if (wrapperType == HDEEM)
		{
			HDEEM::getSamples(_samples);
			if(_samples.nb_blade_values == 0)
			{
				HDEEM::deleteSamples(_samples);
				std::cerr << "MERIC ERROR: No HDEEM samples\n";
				throw 1;
			}
		}
		else if (wrapperType == JETSON)
			JETSON::getSamples(_samplesJetson);
		clock_gettime(CLOCK_REALTIME, &time2);
		MERIC_INFO << "MERIC GET SAMPLES: " << mutils::timeToDouble(time2 - time1) <<" s\n";

		iterate(_records, processInterval);

		if (wrapperType == HDEEM)
		{
			clock_gettime(CLOCK_REALTIME, &time1);
			HDEEM::deleteSamples(_samples);
			clock_gettime(CLOCK_REALTIME, &time2);
			MERIC_INFO << "MERIC DELETE HDEEM SAMPLES: " << mutils::timeToDouble(time2 - time1)<<" s\n";
		}
	}

	TimeStore(const std::string wrapper, bool samples, bool write): Store(samples, write)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);

		if (wrapper == HDEEM::storeLabel)
			wrapperType = HDEEM;
		else if (wrapper == JETSON::storeLabel)
			wrapperType = JETSON;
	}

	TimeStore(const std::string wrapper, bool samples): Store(samples)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);

		if (wrapper == HDEEM::storeLabel)
			wrapperType = HDEEM;
		else if (wrapper == JETSON::storeLabel)
			wrapperType = JETSON;
	}

	TimeStore(const std::string wrapper)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);

		if (wrapper == HDEEM::storeLabel)
			wrapperType = HDEEM;
		else if (wrapper == JETSON::storeLabel)
			wrapperType = JETSON;
	}
	
	TimeStore(bool samples): Store(samples)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);

		wrapperType = HDEEM; //default
	}


/*	TimeStore(bool write = true): Store(write)
	{
		MERIC_INFO << "Create TimeStore\n";
		clock_gettime(CLOCK_REALTIME, &_start);
	}
*/
protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	void computePeriods();

	int wrapperType;
	timespec _start;
	hdeem_global_reading_t _samples;
	std::vector<jetsonSample> _samplesJetson;
	size_t _timeAdjustment;
	int _samples_freq;

	std::vector<TimeRecord> _records;
};

}



#endif /* SRC_BASIS_TIMESTORE_H_ */
