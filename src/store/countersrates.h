/**
 * Class based on https://github.com/RRZE-HPC/likwid/tree/master/groups/haswellEP
 **/


#ifndef SRC_BASIS_COUNTERSRATES_H_
#define SRC_BASIS_COUNTERSRATES_H_


namespace meric {

class CountersRates {

public:
	//L3 miss rate = MEM_LOAD_UOPS_RETIRED_L3_MISS/UOPS_RETIRED_ALL
	double L3_MISS_RATE(long long miss, long long all) { return miss/all; };
	
	//L3 miss ratio = MEM_LOAD_UOPS_RETIRED_L3_MISS/MEM_LOAD_UOPS_RETIRED_L3_ALL
	double L3_MISS_RATIO(long long miss, long long all) { return miss/all; };
	
	//Packed SP MFLOP/s = 1.0E-06*(AVX_INSTS_CALC*8)/runtime;
	//Packed DP MFLOP/s = 1.0E-06*(AVX_INSTS_CALC*4)/runtime;
	double FLOPS(long long avx, long long time) {return 1.0E-06*(avx*8)/time; };

};

}


#endif /* SRC_BASIS_COUNTERSRATES_H_ */
