#ifndef SRC_BASIS_MSRUTIL_H_
#define SRC_BASIS_MSRUTIL_H_

#ifdef HAVE_MSR_CORE_H
	#include <msr_core.h>
#else
	#include <sys/types.h>
	#include <cstdint>
#endif

namespace meric {

int read_msr(int cpuid, off_t MSR_REGISTER_address, uint64_t *MSR_REGISTER_bits);
int write_msr(int cpuid, off_t MSR_REGISTER_address, uint64_t MSR_REGISTER_bits);
}

#endif //SRC_BASIS_MSRUTIL_H_
