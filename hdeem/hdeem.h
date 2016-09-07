#ifndef HDEEM_H
#define HDEEM_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef HDEEM_DRV_H

#ifndef _XOPEN_SOURCE
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */
#endif

#include <time.h>
#include <stdint.h>
#include <string.h>
#endif

#define HDEEM_VERSION "2.2.2"

#define StartedBy(status)			((status >> 1 & 1) ? "IPMI" : "GPIO")
#define IsStartedByIpmi(status)			(status >> 1 & 1)
#define IsFpgaPolling(status)                   (status & 1)
#define IsUploadingSessionActive(status)	(status >> 2 & 1)
#define IsBmcPolling(status)			(status >> 3 & 1)
#define IsBmcOverflow(status)			(status >> 4 & 1)
#define IsFpgaBladeOverflow(status)		(status >> 5 & 1)
#define IsFpgaVrOverflow(status)		(status >> 6 & 1)

#define HDEEM_FPGA_OVERFLOW_GLOBAL                      0x0001
#define HDEEM_FPGA_OVERFLOW_VR                          0x0002
#define HDEEM_ERROR_IPMI_CONNECT                        0x0088
#define HDEEM_ERROR_HDEEMD                              0x008C
#define HDEEM_ERROR_PCIE_CONNECT                        0x0008
#define HDEEM_ERROR_GPIO_INIT                           0x0010
#define HDEEM_ERROR_GPIO_ALREADY_START                  0x0015
#define HDEEM_ERROR_GPIO_CANNOT_START                   0x0016
#define HDEEM_ERROR_GPIO_ALREADY_STOP                   0x0017
#define HDEEM_ERROR_GPIO_CANNOT_STOP                    0x0018
#define HDEEM_ERROR_IPMI_CANNOT_STOP                    0x0019
#define HDEEM_ERROR_ALREADY_RUNNING                     0x0020
#define HDEEM_ERROR_ALREADY_STOPPED                     0x0025
#define HDEEM_ERROR_COMMAND_CONFLICT                    0x0030
#define HDEEM_ERROR_INVALID_RESERVATION_IDENTIFIER      0x0050
#define HDEEM_ERROR_DATA_LENGTH_INVALID                 0x0060 
#define HDEEM_ERROR_OUT_OF_RANGE                        0x0070
#define HDEEM_ERROR_NOUPLOAD_SESSION_ACTIVE             0x0090
#define HDEEM_ERROR_UNKNOWN                             0x0100
#define HDEEM_ERROR_DATALOST_BYBMC                      0x0110
#define HDEEM_ERROR_START_UPLOAD_CONFLICT               0x0120
#define HDEEM_ERROR_CONNECT                             0x0130
#define HDEEM_ERROR_UNABLE_ALLOCATE_MEMORY              0x0140
#define HDEEM_ERROR_INVALID_PARAMETER                   0x0145
#define HDEEM_ERROR_BAD_COUNT_STRUCTURES		        0x0150
#define HDEEM_ERROR_BAD_SIZE_STRUCTURE			        0x0155
#define HDEEM_ERROR_BMC_POLLING_STILL_RUNNING           0x0160
#define HDEEM_ERROR_BAD_BLADE_SAMPLE_SIZE               0x0165
#define HDEEM_ERROR_BAD_VR_SAMPLE_SIZE                  0x0170
#define HDEEM_ERROR_BAD_OUTPUT_STRUCTURE_VERSION        0x0175
#define HDEEM_ERROR_NO_TIME_FILE                        0x0180
#define HDEEM_ERROR_VERSION_MISMATCH                    0x0185

#define HDEEM_ERROR_BAD_COMMAND                         0x0040
#define HDEEM_ERROR_INVALID_COMMAND                     0x0045
#define HDEEM_INTERNAL_ERROR                            0xffff
#define HDEEM_BMC_INTERNAL_ERROR                        0x0080
#define HDEEM_UNKNOWN_ERROR                             0x0085

#define IPMI_RAW_GETPWRCONSUMPTION                      0x005A
#define IPMI_RAW_START                                  0x0071
#define IPMI_RAW_INFO                                   0x0072
#define IPMI_RAW_UPLOAD_ID                              0x0073
#define IPMI_RAW_READ                                   0x0074

#define HDEEM_IPMI_STARTED                              0x0075
#define HDEEM_UPLOAD_SESSION_ACTIVE                     0x0076
#define HDEEM_NEW_SAMPLES_PROBABLY_AVAILABLE            0x0077
#define HDEEM_BMC_MEMORY_FULL                           0x0078
#define HDEEM_FPGA_HARDWARE_ERROR_OR_POLLING_STOPPED    0x0079

#define HDEEM_GPIO34                                    0x0080

// #include <sys/io.h>

#define startStopTimeFilePath "/var/log/hdeem"


/* value of the register  GPIO-BASE  */
#define GPIOBASE_VALUE   0x0500

/* in the following  registers one bit for a GPIO */
/* - register to select GPIO */
#define GP_USE_SEL    0x00
#define GP_USE_SEL2   0x30

/* - register  for Input or Output select */
#define GP_IO_SEL     0x04
#define GP_IO_SEL2    0x34

/* - register to read/write  value of the GPIO */
#define GP_LVL        0x0C
#define GP_LVL2       0x38

/* bit position of GPIO in its register */

/* GPIO-34 in  regisiter GP_<...>2 (eg: GP_USE_SEL2) */
/*   (34 - 32) = 2   */
#define GPIO34         2

/* GPIO-56 in  regisiter GP_<...>2 (eg: GP_USE_SEL2) */
/*   (56 - 32) = 24  */
#define GPIO56        24

#define DF_bit_gpio( register, gpioBit )   ((register >> gpioBit ) & 1)

/* hdeem status*/
typedef struct hdeem_status{
	unsigned char status;
	struct timespec start_time_blade;
	struct timespec start_time_vr;
	struct timespec stop_time_blade;
	struct timespec stop_time_vr;
	uint64_t total_blade_values;
	uint64_t total_vr_values;
	uint64_t pending_blade_values;
	uint64_t pending_vr_values;
} hdeem_status_t;


#ifndef HDEEM_DRV_H

/* Driver do not need that. */

#include <freeipmi/freeipmi.h>
#include <freeipmi/api/ipmi-api.h>


/* Describe how to connect to the BMC */
typedef struct hdeem_bmc_data {
	char * host;
	char * user;
	char * password;
	int hasGPIO;
	int hasPCIe;
	ipmi_ctx_t ctx;
	int nb_vr_sensors;
	int nb_blade_sensors;
	char ** name_vr_sensors;
	char ** name_blade_sensors;
	int structure_version;
	int blade_frequency;
	int vr_frequency;
	struct timespec skew_blade;
	struct timespec skew_vr;
} hdeem_bmc_data_t;

/* One snapshot of the blade power sensor(s) */
typedef struct hdeem_blade_power {
	float *value;
} hdeem_blade_power_t;

/* One snapshot of the VR power sensors */
typedef struct hdeem_vr_power {
	float *value;
} hdeem_vr_power_t;

/* One snapshot of the VR power sensors */
typedef struct hdeem_energy {
	double *value;
} hdeem_energy_t;

/* All blade and VR measured values from start to stop */ 
typedef struct hdeem_global_reading {
	uint64_t first_blade_value;
	uint64_t first_vr_value;
	uint64_t nb_blade_values;
	uint64_t nb_vr_values;
	hdeem_blade_power_t  * blade_power;
	hdeem_vr_power_t * vr_power;
} hdeem_global_reading_t;

/* Statistics about min, max and average power between start and stop */
typedef struct hdeem_stats_reading {
	hdeem_vr_power_t max_vr_values;
	hdeem_blade_power_t max_blade_values;
	hdeem_vr_power_t min_vr_values;
	hdeem_blade_power_t min_blade_values;
	hdeem_energy_t energy_vr_values;
	hdeem_energy_t energy_blade_values;

	hdeem_vr_power_t average_vr_values;
	hdeem_blade_power_t average_blade_values;

    uint64_t nb_blade_values;
    uint64_t nb_vr_values;

	hdeem_vr_power_t instant_vr_values;
    hdeem_blade_power_t instant_blade_values;
	
    struct timespec read_time_blade;
    struct timespec read_time_vr;
} hdeem_stats_reading_t;

#define hdeem_init(bmc) __hdeem_init(bmc, HDEEM_VERSION) 
int __hdeem_init(hdeem_bmc_data_t *, const char *);

/*
	Create ipmi context and open a connection to the host specified in the hdeem_bmc_t structure
*/

void hdeem_close(hdeem_bmc_data_t *bmc);
/*
 	Close and destroy ipmi context
*/

int hdeem_start (hdeem_bmc_data_t * bmc);
/*
	- for out of band connection

	*bmc must contain :
	bmc -> host = ip address of the bmc 
	bmc -> user = "super"
	bmc -> pass = "pass"
	bmc - > ctx = ipmi_ctx_create() // create by a call to the freeipmi library prior to this call

	calls ipmi_ctx_open_outofband(...)


	- for in band connection
	bmc -> host = ""
	bmc - > ctx = ipmi_ctx_create()

	- rfu: reserved for future use
	In a next version of this interface, the rate of power values per second may be tunable.


	calls ipmi_ctx_open_inband(...)
*/

int hdeem_stop(hdeem_bmc_data_t * bmc);


int hdeem_check_status(hdeem_bmc_data_t * bmc, hdeem_status_t * readStatus);
/* Check for running FPGA and BMC polling
 *    Do not retrieve power values
 */

int hdeem_get_global(hdeem_bmc_data_t * bmc, hdeem_global_reading_t * readings );

int hdeem_get_stats_total(hdeem_bmc_data_t * bmc, hdeem_stats_reading_t * stats );

int hdeem_get_stats(hdeem_bmc_data_t * bmc, hdeem_stats_reading_t * stats );
	
void hdeem_data_free(hdeem_global_reading_t * readings);
/*
 * free: 
 *  stats->average_blade_values.value   
 *  stats->energy_blade_values.value    
 *  stats->min_blade_values.value       
 *  stats->max_blade_values.value       
 *  stats->average_vr_values.value      
 *  stats->energy_vr_values.value       
 *  stats->min_vr_values.value         
 *  stats->max_vr_values.value 
 * allocated by hdeem_get_stats
 *
 */

void hdeem_stats_free(hdeem_stats_reading_t * stats);
/*
 * free readings.blade_power and readings.vr_power allocated by hdeem_get_global
 *
 */


int hdeem_clear(hdeem_bmc_data_t * bmc);
/*
 * Stop or force stop upload session and stop or force stop high speed power consumption data collection. The reservation id of upload session will be lost.
 *
 */

void __hdeem_version(char *, int);
#define hdeem_version(buf , n) { __hdeem_version(buf, n) ; if (strcmp(HDEEM_VERSION, buf)) fprintf(stderr, "Possible version mismatch, compiled %s <> linked %s\n", HDEEM_VERSION, buf); }
/*
 * HDEEM VERSION
 */
#endif
 
#ifdef __cplusplus
}
#endif

#endif /* HDEEM_H*/

