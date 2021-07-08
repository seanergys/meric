#ifndef SRC_BASIS_MSRUTIL_H_
#define SRC_BASIS_MSRUTIL_H_

namespace meric {

int read_msr(int cpuid, off_t MSR_REGISTER_address, uint64_t *MSR_REGISTER_bits);
int write_msr(int cpuid, off_t MSR_REGISTER_address, uint64_t MSR_REGISTER_bits);
}

#endif //SRC_BASIS_MSRUTIL_H_
