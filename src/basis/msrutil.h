#ifndef SRC_BASIS_MSRUTIL_H_
#define SRC_BASIS_MSRUTIL_H_

#ifdef HAVE_MSR_CORE_H
	#include <msr_core.h>
#else
	#include <sys/types.h>
	#include <cstdint>
	#include <stdio.h>
	#include <fcntl.h>
	#include <iostream>
	#include <unistd.h>
#endif

namespace meric {

static int read_msr(int cpuid, off_t MSR_REGISTER_address, uint64_t *MSR_REGISTER_bits)
{
#ifdef HAVE_MSR_CORE_H
	return read_msr_by_idx(cpuid, MSR_REGISTER_address, MSR_REGISTER_bits);
#else
	char msr_file_name[64];
	sprintf(msr_file_name, "/dev/cpu/%d/msr_safe", cpuid);
	int fd = open(msr_file_name, O_RDONLY);
	if (fd < 0)
	{
		std::cerr << "read msr error ["<<cpuid<<"]\n";
		return -1;
	}

	if (pread(fd, MSR_REGISTER_bits, sizeof MSR_REGISTER_bits, MSR_REGISTER_address) != sizeof MSR_REGISTER_bits)
	{
		std::cerr << "read msr error - cannot read register " << std::hex << MSR_REGISTER_address << "\n";
		return -1;
	}
	close(fd);
	return 0;
#endif
}

static int write_msr(int cpuid, off_t MSR_REGISTER_address, uint64_t MSR_REGISTER_bits)
{
#ifdef HAVE_MSR_CORE_H
	return write_msr_by_idx(cpuid, MSR_REGISTER_address, MSR_REGISTER_bits);
#else

	char msr_file_name[64];
	sprintf(msr_file_name, "/dev/cpu/%d/msr_safe", cpuid);
	int fd = open(msr_file_name, O_WRONLY);
	if (fd < 0)
	{
		std::cerr << "write msr error ["<<cpuid<<"]\n";
		return -1;
	}
	
	if (pwrite(fd, &MSR_REGISTER_bits, sizeof MSR_REGISTER_bits, MSR_REGISTER_address) != sizeof MSR_REGISTER_bits)
	{
		std::cerr << "write msr error - cannot write register " << std::hex << MSR_REGISTER_address << "\n";
		return -1;
	}
	return 0;
#endif
}


}

#endif //SRC_BASIS_MSRUTIL_H_
