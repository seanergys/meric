#ifndef SRC_BASIS_MSRUTIL_H_
#define SRC_BASIS_MSRUTIL_H_

#ifdef HAVE_MSR_CORE_H
	#include <msr_core.h>
#endif

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/types.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

struct msr_batch_op
{
	__u16 cpu;	   // In: CPU to execute {rd/wr}msr instruction
	__u16 isrdmsr; // In: 0=wrmsr, non-zero=rdmsr
	__s32 err;	   // Out: set if error occurred with this operation
	__u32 msr;	   // In: MSR Address to perform operation
	__u64 msrdata; // In/Out: Input/Result to/from operation
	__u64 wmask;   // Out: Write mask applied to wrmsr
};

struct msr_batch_array
{
	__u32 numops;			  // In: # of operations in operations array
	__u32 version;			  // In: MSR_SAFE_VERSION_u32 (see msr_version.h)
	struct msr_batch_op *ops; // In: Array[numops] of operations
};

#define X86_IOC_MSR_BATCH _IOWR('c', 0xA2, struct msr_batch_array)

#include "sysfsutils.h"
#include <vector>

namespace meric {

static int read_msr(int cpuid, off_t MSR_REGISTER_address, uint64_t *MSR_REGISTER_bits)
{
#ifdef HAVE_MSR_CORE_H
	return read_msr_by_idx(cpuid, MSR_REGISTER_address, MSR_REGISTER_bits);
#else
	char msr_file_name[64];
	sprintf(msr_file_name, "/dev/cpu/%d/msr_safe", cpuid);
	sysfsutils::UniqueFdHandle fd(open(msr_file_name, O_RDONLY));
	if (!fd.IsValid())
	{
		std::cerr << "MERIC ERROR: read msr error [" << cpuid << "] - cannot open FD\n";
		return -1;
	}

	const bool res = pread(fd, MSR_REGISTER_bits, sizeof(MSR_REGISTER_bits), MSR_REGISTER_address) == sizeof(MSR_REGISTER_bits);

	if (!res)
	{
		std::cerr << "MERIC ERROR:  read msr error - cannot read register " << std::hex << MSR_REGISTER_address << "\n";
		return -1;
	}

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
	sysfsutils::UniqueFdHandle fd(open(msr_file_name, O_WRONLY));
	if (!fd.IsValid())
	{
		std::cerr << "MERIC ERROR: write msr error [" << cpuid << "] - cannot open FD\n";
		return -1;
	}
	
	const bool res = pwrite(fd, &MSR_REGISTER_bits, sizeof(MSR_REGISTER_bits), MSR_REGISTER_address) == sizeof(MSR_REGISTER_bits);

	if (!res)
	{
		std::cerr << "MERIC ERROR: write msr error - cannot write register " << std::hex << MSR_REGISTER_address << "\n";
		return -1;
	}

	return 0;
#endif
}

static int submit_msr_batch(std::vector<struct msr_batch_op> &ops)
{
	static sysfsutils::UniqueFdHandle fd(open("/dev/cpu/msr_batch", O_RDWR));
	if (!fd.IsValid())
	{
		std::cerr << "MERIC ERROR: write msr batch error - cannot open /dev/cpu/msr_batch" << std::endl;
		return -1;
	}

	struct msr_batch_array batch;
	batch.numops = ops.size();
	batch.version = 0; // NOTE: Should not be checked until msr-safe 2.0
	batch.ops = ops.data();

	if (ioctl(fd, X86_IOC_MSR_BATCH, &batch) != 0)
	{
		std::cerr << "MERIC ERROR: msr batch submit error - " << strerror(errno) << std::endl;
		return -1;
	}

	return 0;
}

/**
 * @brief Read MSR register at `MSR_REGISTER_address` of each CPU ID specified in `cpus` into corresponding slot in `MSR_REGISTER_bits`.
 *
 * @param cpus 					List of CPU ids
 * @param MSR_REGISTER_address  Address of the MSR register to be read
 * @param MSR_REGISTER_bits     Value of the MSR register for each specified CPU
 * @return int 					Zero on success, non-zero otherwise
 */
static int read_msr_batch(const std::vector<int> &cpus, off_t MSR_REGISTER_address, std::vector<uint64_t> &MSR_REGISTER_bits)
{
	std::vector<struct msr_batch_op> ops(cpus.size());
	for (size_t i = 0; i < cpus.size(); ++i)
	{
		ops[i].cpu = cpus[i];
		ops[i].isrdmsr = 1;
		ops[i].err = 0;
		ops[i].msr = MSR_REGISTER_address;
		ops[i].msrdata = 0;
		ops[i].wmask = 0;
	}

	if (submit_msr_batch(ops) != 0)
	{
		return -1;
	}

	MSR_REGISTER_bits.resize(ops.size(), 0);
	for (size_t i = 0; i < ops.size(); ++i)
	{
		MSR_REGISTER_bits[i] = ops[i].msrdata;

		if (ops[i].err)
		{
			std::cerr << "MERIC ERROR: read msr batch error - cannot read register " << std::hex << MSR_REGISTER_address << std::dec << " on CPU " << ops[i].cpu
					  << ": " << strerror(ops[i].err < 0 ? -ops[i].err : ops[i].err) << std::endl;
			return -1;
		}
	}

	return 0;
}

/**
 * @brief Write *uniform* value `MSR_REGISTER_bits` to MSR register at `MSR_REGISTER_address` of each CPU specified in `cpus`
 *
 * @param cpus 					List of CPU ids
 * @param MSR_REGISTER_address 	Address of the MSR register to be written
 * @param MSR_REGISTER_bits 	Uniform value to be written to MSR of each CPU
 * @return int 					Zero on success, non-zero otherwise
 */
static int write_msr_batch(const std::vector<int> &cpus, off_t MSR_REGISTER_address, uint64_t MSR_REGISTER_bits)
{
	std::vector<struct msr_batch_op> ops(cpus.size());
	for (size_t i = 0; i < cpus.size(); ++i)
	{
		ops[i].cpu = cpus[i];
		ops[i].isrdmsr = 0;
		ops[i].err = 0;
		ops[i].msr = MSR_REGISTER_address;
		ops[i].msrdata = MSR_REGISTER_bits;
		ops[i].wmask = 0;
	}

	if (submit_msr_batch(ops) != 0)
	{
		return -1;
	}

	for (const auto &op : ops)
	{
		if (op.err)
		{
			std::cerr << "MERIC ERROR: write msr batch error - cannot write register " << std::hex << MSR_REGISTER_address << std::dec << " on CPU " << op.cpu
					  << ": " << strerror(op.err < 0 ? -op.err : op.err) << std::endl;
			return -1;
		}
	}

	return 0;
}

/**
 * @brief Non-uniform write of MSR register at `MSR_REGISTER_address`.
 *
 * @param cpu_bits 				List of <CPU_ID, value> pair specifying value to be written on each CPU
 * @param MSR_REGISTER_address 	Address of the MSR register to be written
 * @return int 					Zero on success, non-zero otherwise
 */
static int write_msr_batch(const std::vector<std::pair<int, uint64_t>> &cpu_bits, off_t MSR_REGISTER_address)
{
	std::vector<struct msr_batch_op> ops(cpu_bits.size());
	for (size_t i = 0; i < cpu_bits.size(); ++i)
	{
		ops[i].cpu = cpu_bits[i].first;
		ops[i].isrdmsr = 0;
		ops[i].err = 0;
		ops[i].msr = MSR_REGISTER_address;
		ops[i].msrdata = cpu_bits[i].second;
		ops[i].wmask = 0;
	}

	if (submit_msr_batch(ops) != 0)
	{
		return -1;
	}

	for (const auto &op : ops)
	{
		if (op.err)
		{
			std::cerr << "MERIC ERROR: write msr batch error - cannot write register " << std::hex << MSR_REGISTER_address << std::dec << " on CPU " << op.cpu
					  << ": " << strerror(op.err < 0 ? -op.err : op.err) << std::endl;
			return -1;
		}
	}

	return 0;
}

bool check_whitelist_msr(const std::vector<std::pair<uint64_t, uint64_t> > &regs);

}

#endif //SRC_BASIS_MSRUTIL_H_
