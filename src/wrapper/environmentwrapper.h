#ifndef SRC_WRAPPER_ENVIRONMENTWRAPPER_H_
#define SRC_WRAPPER_ENVIRONMENTWRAPPER_H_

#include "../meric/meric.h"

#ifdef HAVE_CPUFREQ_H
#include "cpufreq.h"
#endif

#ifdef HAVE_X86_ADAPT_H
#include <stdint.h>
#include <memory>
#include "x86_adapt.h"
#endif

#ifdef HAVE_CILK_CILK_API_H
#include <cilk/cilk_api.h> 
#endif

#ifdef HAVE_OMP_H
#include <omp.h>
#endif
#include <unistd.h>


namespace meric {

const int CPUFREQ_FREQ_MULT = 100000;
const int X86ADAPT_FREQ_MULT = 256;
const int MAX_FREQ = 25;
const int X86ADAPT_MAX_FREQ = 33;
const int MAX_UNCORE_FREQ = 30;


class Environment {

public:

	static void frequency(uint64_t freq)
	{

#ifdef HAVE_CPUFREQ_H
		if (freq <= MAX_FREQ)
		{
			MERIC_INFO << "CPUFREQ set frequency to: " << freq << "\n";
			int ret;
			const int nodeSize = sysconf(_SC_NPROCESSORS_ONLN);
			for (int i=0; i < nodeSize; i++)
			{
				ret = cpufreq_set_frequency(i,freq*CPUFREQ_FREQ_MULT);

				if(ret != 0 || freq*CPUFREQ_FREQ_MULT != cpufreq_get_freq_kernel(i))
				{
					std::cerr << "CPUFREQ ERROR - failed to set frequency for cpu "<< i << std::endl;
	//				MPI_Abort(MPI_COMM_WORLD, 1);
				}
			}
		}
		else
#endif
#ifdef HAVE_X86_ADAPT_H
		{	//x86adapt allows to set cpu frequency ut to 3.3 GHz
			if (freq > X86ADAPT_MAX_FREQ)
			{
				std::cerr << "CPU frequency did not changed to: " << freq << std::endl;
				return;
			}
			if (x86_adapt_lookup_ci_name(X86_ADAPT_CPU, "Intel_Target_PState") < 0)
				return;
			MERIC_INFO << "X86ADAPT set frequency to: " << freq << "\n";

			const int max = x86_adapt_get_nr_avaible_devices(X86_ADAPT_CPU);
			for (int cpu=0; cpu<max; cpu++)
			{
				int fd = x86_adapt_get_device(X86_ADAPT_CPU, cpu);
				if (fd < 0)
				{
					std::cerr << "X86ADAPT error while accessing file descriptor\n";
	//				exit(2);
				}

				if(x86_adapt_set_setting(fd, x86_adapt_lookup_ci_name(X86_ADAPT_CPU, "Intel_Target_PState"),freq*X86ADAPT_FREQ_MULT) != 8)
				{
					std::cerr << "X86ADAPT error while setting core frequecy\n";
	//					exit(3);
				}
			
				if ( x86_adapt_put_device(X86_ADAPT_CPU, cpu))
					std::cerr << "X86ADAPT error while closing file descriptor\n";
			}
		}
#else
		{
			std::cerr << "CPU frequency did not changed to: " << freq << std::endl;
		}
#endif
	}
	
	static void uncoreFrequency(uint64_t freq)
	{
#ifdef HAVE_X86_ADAPT_H
		if (x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO") < 0)
			return;

		MERIC_INFO << "X86ADAPT set uncore frequency to: "<< freq << std::endl;
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
			
			if ( x86_adapt_put_device(X86_ADAPT_DIE, die))
				std::cerr << "X86ADAPT error while closing file descriptor\n";
		}
#else
#endif
	}
	
	static void threadNumber(int nthreads)
	{
		if (nthreads <= 0)
			return;
/*		int running = 0;
		running = omp_get_num_threads();
		if (nthreads != running)
		{
			if (omp_in_parallel())
				std::cerr << "WARNING: MERIC changes number of threads inside a parallel region\n";
			omp_set_num_threads(nthreads);
		}
*/
#ifdef HAVE_OMP_H
		if (!omp_in_parallel())
		{
			MERIC_INFO << "set num threads to: " << nthreads << std::endl;
			omp_set_num_threads(nthreads);
		}
		else
		{
			MERIC_INFO << "MERIC WARNING: paralel region, num threads did not change to: " << nthreads << std::endl;
		}
#endif
/*
#ifdef HAVE_CILK_CILK_API_H
		//using cilk
		__cilkrts_end_cilk();  
		__cilkrts_set_param("nworkers", std::to_string(nthreads).c_str());
#endif
*/
	}
	
	static void init(RegionParameters defaultParameter, int rank)
	{
#ifdef HAVE_X86_ADAPT_H
		if (x86_adapt_init())
		{
			std::cerr << "X86ADAPT init error\n";
//			exit(1);
		}
		
		//test if it is possible to set uncore frequency
		if (x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO") < 0)
			std::cerr << "X86ADAPT WARNING: UNCORE FREQUENCY SETTINGS IS NOT AVAILABLE\n";
#endif		
		if(!rank)
		{
			frequency(defaultParameter.frequency);
			uncoreFrequency(defaultParameter.uncoreFrequency);
		}
		threadNumber(defaultParameter.nthreads);
	}
	
	static void close()
	{
#ifdef HAVE_X86_ADAPT_H
		x86_adapt_finalize();
#endif
	}
	
	
};

}


#endif /* SRC_WRAPPER_ENVIRONMENTWRAPPER_H_ */
