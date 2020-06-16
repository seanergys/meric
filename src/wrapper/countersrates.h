/**
 * Class based on https://github.com/RRZE-HPC/likwid/tree/master/groups/haswellEP
 **/


#ifndef SRC_BASIS_COUNTERSRATES_H_
#define SRC_BASIS_COUNTERSRATES_H_


namespace meric {

class CountersRates {

public:
	//L3 miss rate = MEM_LOAD_UOPS_RETIRED_L3_MISS/UOPS_RETIRED_ALL
	static double L3_MISS_RATE(long long miss, long long all) { return all ? miss/all : 0; };
	
	//L3 miss ratio = MEM_LOAD_UOPS_RETIRED_L3_MISS/MEM_LOAD_UOPS_RETIRED_L3_ALL
	static double L3_MISS_RATIO(long long miss, long long all) { return all ? miss/all : 0; };
	
	//Packed SP GFLOP/s = 1.0E-09*(AVX_INSTS_CALC*8)/runtime;
	//Packed DP GFLOP/s = 1.0E-09*(AVX_INSTS_CALC*4)/runtime;
	static double FLOPS(long long avx, double time) {return (1.0E-09*(avx*8)/time); };
	
	static double computationalIntensity(long long instructions, long long miss) {return miss ? instructions/miss : instructions; };

	//average CPU core frequency in a specified time range [Hz]
	static double CPU_FREQUENCY_FROM_CYCLES(long long cycles, double time) {return cycles/time;};
	static double CPU_FREQUENCY_FROM_Pstate_PERF(uint64_t aperf, uint64_t mperf, uint64_t nominal_freq) { return mperf ? (aperf/(double)mperf)*nominal_freq : 0; };
	
	//average CPU uncore frequency in a specified time range [Hz]
	static double CPU_UNCORE_FREQUNECY(uint64_t cycles, double time) { return cycles ? cycles/time : 0; };
	
	//ratio of time during which the RAPL system was active
	static double RAPL_ACTIVE_RATIO(double cappedTime, double overallTime) { return overallTime ? cappedTime/overallTime : 0;};

};

}


#endif /* SRC_BASIS_COUNTERSRATES_H_ */
