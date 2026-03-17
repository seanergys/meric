
#ifndef SRC_WRAPPER_OCCWRAPPER_H_
#define SRC_WRAPPER_OCCWRAPPER_H_

#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <locale>
#include <unistd.h>
#include <fcntl.h>
#include <endian.h> //be32toh()

#include "counters.h"
#include "../basis/utils.h"

#define TO_FP(f)    ((f >> 8) * pow(10, ((int8_t)(f & 0xFF))))
#define OCC_INBAND_SENSORS 				"/sys/firmware/opal/exports/occ_inband_sensors"

#define MAX_OCCS						8
#define MAX_CHARS_SENSOR_NAME			16
#define MAX_CHARS_SENSOR_UNIT			4

#define OCC_SENSOR_DATA_BLOCK_OFFSET	0x00580000
#define OCC_SENSOR_DATA_BLOCK_SIZE		0x00025800

// OCC Firmware Interface Specification for POWER9 (version 0.24): Section 11.3.1.2 Sensor Names 
enum occ_sensor_type {
	OCC_SENSOR_TYPE_GENERIC	= 			0x0001,
	OCC_SENSOR_TYPE_CURRENT	= 			0x0002,
	OCC_SENSOR_TYPE_VOLTAGE	= 			0x0004,
	OCC_SENSOR_TYPE_TEMPERATURE	= 		0x0008,
	OCC_SENSOR_TYPE_UTILIZATION	= 		0x0010,
	OCC_SENSOR_TYPE_TIME = 				0x0020,
	OCC_SENSOR_TYPE_FREQUENCY = 		0x0040,
	OCC_SENSOR_TYPE_POWER = 			0x0080,
	OCC_SENSOR_TYPE_PERFORMANCE	= 		0x0200,
};

enum occ_sensor_location {
	OCC_SENSOR_LOC_SYSTEM = 			0x0001,
	OCC_SENSOR_LOC_PROCESSOR = 			0x0002,
	OCC_SENSOR_LOC_PARTITION = 			0x0004,
	OCC_SENSOR_LOC_MEMORY = 			0x0008,
	OCC_SENSOR_LOC_VRM = 				0x0010,
	OCC_SENSOR_LOC_OCC = 				0x0020,
	OCC_SENSOR_LOC_CORE = 				0x0040,
	OCC_SENSOR_LOC_GPU = 				0x0080,
	OCC_SENSOR_LOC_QUAD = 				0x0100,
};

enum sensor_struct_type {
	OCC_SENSOR_READING_FULL = 			0x01,
	OCC_SENSOR_READING_COUNTER = 		0x02,
};

typedef struct {
	uint8_t valid;
	uint8_t version;
	uint16_t nr_sensors;
	uint8_t reading_version;
	uint8_t pad[3];
	uint32_t names_offset;
	uint8_t names_version;
	uint8_t name_length;
	uint16_t reserved;
	uint32_t reading_ping_offset;
	uint32_t reading_pong_offset;
} __attribute__((__packed__)) occ_sensor_data_header_t;

typedef struct {
	char name[MAX_CHARS_SENSOR_NAME];
	char units[MAX_CHARS_SENSOR_UNIT];
	uint16_t gsid;
	uint32_t freq;
	uint32_t scale_factor;
	uint16_t type;
	uint16_t location;
	uint8_t structure_type;
	uint32_t reading_offset;
	uint8_t sensor_data;
	uint8_t pad[8];
} __attribute__((__packed__)) occ_sensor_name_t;

typedef struct {
	uint16_t gsid;
	uint64_t timestamp;
	uint16_t sample;
	uint16_t sample_min;
	uint16_t sample_max;
	uint16_t csm_min;
	uint16_t csm_max;
	uint16_t profiler_min;
	uint16_t profiler_max;
	uint16_t job_scheduler_min;
	uint16_t job_scheduler_max;
	uint64_t accumulator;
	uint32_t update_tag;
	uint8_t pad[8];
} __attribute__((__packed__)) occ_sensor_record_t;

using namespace OCC_COUNTERS;

namespace meric {

class OCC {

public:
	static void init(bool detailed = false);
	static void close()
	{
		::close(OCC_fd);
	};
	static void getCounterValues(std::map<std::string, uint64_t> & record);
	static void initMap(std::map<std::string, uint64_t> & dict);
	static uint64_t getResultValue(uint64_t startValue, uint64_t stopValue, double runtime, int & cumulative, std::string counter, std::string regionName = "");
	static inline std::string getSummaryCounterName() { return ""; }; //no artifitial counter
	static  inline std::string getSourceLabel() { return "OCC"; };
	static std::string getCounterLabel(std::string counter);
	static void getRatios(std::map<std::string, uint64_t>& start, std::map<std::string, uint64_t>& stop, double runtime, std::map <std::string, double> & ratios) {};

	static bool isAvailable();

private:
	static std::map<std::string, uint64_t> counters;
	static std::string suffix;
	static bool detailedMode;
	static int OCC_fd;
};

}



#endif /* SRC_WRAPPER_OCCWRAPPER_H_ */
