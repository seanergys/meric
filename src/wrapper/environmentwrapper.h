#ifndef SRC_WRAPPER_ENVIRONMENTWRAPPER_H_
#define SRC_WRAPPER_ENVIRONMENTWRAPPER_H_

#ifndef USE_DUMMY

#ifdef HAVE_CPUFREQ_H
#include "cpufreq.h"
#endif

#ifdef HAVE_X86_ADAPT_H
#include <stdint.h>
#include <memory>
#include "x86_adapt.h"
#endif

const int NODE_SIZE = 24; //TODO

#endif

namespace meric {

class Environment {

public:

	static void frequency(uint64_t freq)
	{
		MERIC_INFO << "change frequency to " << freq << "\n";
#ifdef USE_DUMMY
		std::cout << "set frequency to: " << freq << "\n";
#elif HAVE_CPUFREQ_H
		std::cout << "set frequency to: " << freq << "\n";
		int ret;
		for (int i=0; i < NODE_SIZE; i++)
		{
//			std::cout << "set freq "<<freq<<" for cpu "<<i<<std::endl;
			ret = cpufreq_set_frequency(i,freq);

			if(ret != 0 || freq != cpufreq_get_freq_kernel(i))
			{
				std::cerr << "CPUFREQ ERROR - failed to set frequency for cpu "<< i << std::endl;
//				MPI_Abort(MPI_COMM_WORLD, 1);
			}
		}
#else
#endif
	}
	
	static void uncoreFrequency(uint64_t freq)
	{
		MERIC_INFO << "change uncore frequency to " << freq << std::endl;
#ifdef USE_DUMMY
		std::cout << "set uncore frequency to: " << freq << std::endl;
#elif HAVE_X86_ADAPT_H
		std::cout << "set uncore frequency to: "<< freq << std::endl;
		const int max = x86_adapt_get_nr_avaible_devices(X86_ADAPT_DIE);
		for (int die=0; die<max; die++)
		{
			int fd = x86_adapt_get_device(X86_ADAPT_DIE, die);
			if (fd < 0)
			{
				std::cerr << "X86ADAPT error while accessing file descriptor\n";
//				exit(2);
			}

			auto setFreq = [&] (const char * description)
			{
				if(x86_adapt_set_setting(fd, x86_adapt_lookup_ci_name(X86_ADAPT_DIE, description),freq ) != 8)
				{
					std::cerr << "X86ADAPT error while setting uncore frequecy\n";
//					exit(3);
				}
			};

			setFreq("Intel_UNCORE_MAX_RATIO");
			setFreq("Intel_UNCORE_MIN_RATIO");
		}
#else
#endif
	}
	
	static void init()
	{
		if (x86_adapt_init())
		{
			std::cerr << "X86ADAPT init error\n";
//			exit(1);
		}
	}
	
	static void close()
	{
		x86_adapt_finalize();
	}
	
	
};

}


#endif /* SRC_WRAPPER_ENVIRONMENTWRAPPER_H_ */
