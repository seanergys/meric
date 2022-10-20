# MERICwrapper #

MERICwrapper is a tool for state-space-search using MERIC library. The MERIC provides hadrware and system-software parameters tuning, however it does not decide in what configuration to execute. The MERICwrapper based on specified space of parameter to tune, executes the application based on one of implemented algorithms for state-space-search. Output of the MERICwrapper is a MERIC configuration file, that contains optimal configuration of the tuned parameters for each region of the application, which is used for future executions of the application with optimised energy-efficiency.

--------------------------------------------------------------------------------
#     1] Input parameters                                                      #
--------------------------------------------------------------------------------

Example usage:
```
./wrapper -s config.json -- mpirun -np 2 ./a.out -a -x
```

List of supported parameters:
 * `-d` = generate default config file 'defaultWrapperConfig.json'.
 * `-s configFile` = run application analysis according the configuration file.
 * `-a pathToDir` = generate optimal configuration file from a data measured by MERIC.
 	* `-m MERIC_MEASURE` = measurement system that was used to measure the data, only one can be specified, no default.
 * `-b baseline` = power baseline [W] that should be used for analysis, default 0 W.
 * `-g margin` = energy consumption margin [%] of selected configuration, default 0 %. Select a configuration with energy consumption within the margin of the optimal configuration, but with better runtime.
 * `-e extension` = maximum runtime extension [%], default 10 %.
 * `--` = follows command to run the application to analyze.


--------------------------------------------------------------------------------
#     2] Configuration file                                                    #
--------------------------------------------------------------------------------

MERICwrapper JSON configuration file format:

 * `MPI` = `true` or `false`; identification whether the application is using MPI
 * `BASELINE` = float; power baseline [W]
 * `PARAMETERS` = selected parameters tuned by MERIC
 	* parameter name same as uses MERIC in its configuration file
 		* `MAX`, `MIN`, `STEP` = range of parameter values to set, cannot be combined with `LIST`
 		* `LIST` = list of parameter values to set, cannot be combined with `MAX`, `MIN`, `STEP`
 * `ALGORITHM` = parameters of selected state-space-search algorithm
 	* `NAME` = `EVO`, `PSO`, `EXHUATIVE`; selection of state-space-search algorithm
 	* `PARAMETERS` = parameters of the selected algoritm, described later
 * `MERIC` = MERIC parameters
	* `MEASURE`
	* `COUNTERS`
	* `AGGREGATE`
	* `DETAILED`
	* `SAMPLES`
	* `BARRIERS`
	* `OUTPUT_DIR`
	* `FREQUENCY` - if parameter not specified in `PARAMETERS` section
	* `UNCORE_FREQUENCY` - if parameter not specified in `PARAMETERS` section
	* `NUM_THREADS` - if parameter not specified in `PARAMETERS` section
	* `PWRCAP_POWER` - if parameter not specified in `PARAMETERS` section
	* `PWRCAP_TIME` - if parameter not specified in `PARAMETERS` section
	* `GPU_FREQUENCY` - if parameter not specified in `PARAMETERS` section
	* `GPU_MEM_FREQUENCY` - if parameter not specified in `PARAMETERS` section
	* `PWRCAP_GPU` - if parameter not specified in `PARAMETERS` section

## Supported state-space-search algorithms:

### Exhaustive
Execute the analysed application in all the configurations of the specified parameters state-space.

No additional parameters

### Particle Swarm optimisation
Iterative algorithm using a population of candidate solutions moving around in the state-space.

Parametrs:
 * `END_CONDITION` = integer, end the search if the best configuration has not been improved END_CONDITION-times
 * `PARTICLES` = integer, amount of particles in a swarm
 * `CONST_CP` = float, cognitive parameter, requires `CONST_CP` + `CONST_CG` > 4, default 2.05
 * `CONST_CG` = float, social parameter, requires `CONST_CP` + `CONST_CG` > 4, default 2.05
 * `CONST_W` = float, momentum parameter, <0.4, 0.9>, default 0.9

### Evolution algorithm
A genetic algorithm using uniform crossover.

Parameters:
 * `END_CONDITION` = integer, end the search if the best configuration has not been improved END_CONDITION-times
 * `POPULATION` = integer, size of population


## Example configuration file

```
{
	"MPI" : "true",
	"BASELINE" : 60.0,
	
	"PARAMETERS" : {
		"FREQUENCY" : {
			"LIST": [
				2500000000,
				2100000000,
				1800000000,
				1200000000
			]
		},
		"UNCORE_FREQENCY" : {
			"MAX" : 3000000000,
			"MIN" : 1200000000,
			"STEP":  200000000
		}
	},

	"ALGORITHM" : {
		"NAME" : "EVO",
		"PARAMETERS" : {
			"END_CONDITION" : 2,
			"POPULATION" : 10
		}
	},

	"MERIC" : {
		"MEASURE" : RAPL,
		"COUNTERS" : "papi",
		"OUTPUT_DIR" : "mericMeasurement",
		"NUM_THREADS" : 36
	}
}
```

--------------------------------------------------------------------------------
#     3] Acknowledgement                                                       #
--------------------------------------------------------------------------------
MERICwrapper is being developed at [IT4Innovations National Supercomputing Center](https://www.it4i.cz/) under [BSD-3 license](https://code.it4i.cz/vys0053/meric/blob/master/LICENSE).
Please, open an issue, if you meet any problem. 

