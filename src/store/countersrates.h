/**
 * Class based on https://github.com/RRZE-HPC/likwid/tree/master/groups/haswellEP
 **/


#ifndef SRC_BASIS_COUNTERSRATES_H_
#define SRC_BASIS_COUNTERSRATES_H_


namespace meric {

class CountersRates {

public:
	//L3 miss rate = MEM_LOAD_UOPS_RETIRED_L3_MISS/UOPS_RETIRED_ALL
	static double L3_MISS_RATE(long long miss, long long all) { return miss/all; };
	
	//L3 miss ratio = MEM_LOAD_UOPS_RETIRED_L3_MISS/MEM_LOAD_UOPS_RETIRED_L3_ALL
	static double L3_MISS_RATIO(long long miss, long long all) { return miss/all; };
	
	//Packed SP GFLOP/s = 1.0E-09*(AVX_INSTS_CALC*8)/runtime;
	//Packed DP GFLOP/s = 1.0E-09*(AVX_INSTS_CALC*4)/runtime;
	static double FLOPS(long long avx, double time) {return (1.0E-09*(avx*8)/time); };
	
	static double computationalIntensity(long long instructions, long long miss) {return instructions/miss; };

};

}


#endif /* SRC_BASIS_COUNTERSRATES_H_ */
