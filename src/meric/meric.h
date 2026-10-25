
#ifndef SRC_MERIC_MERIC_H_
#define SRC_MERIC_MERIC_H_

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "../basis/cpu_affinity_map.h"
#include "../basis/json.h"
#include "../basis/tokenizer.h"
#include "../basis/utils.h"
#include "../store/countersstore.h"
#include "../store/infostore.h"
#include "../store/samplesstore.h"
#include "../store/timeenergystore.h"

#include "../wrapper/a64fxwrapper.h"
#include "../wrapper/e4wrapper.h"
#include "../wrapper/environmentwrapper.h"
#include "../wrapper/hwmonwrapper.h"
#include "../wrapper/msrcounterswrapper.h"
#include "../wrapper/nvmlwrapper.h"
#include "../wrapper/occwrapper.h"
#include "../wrapper/papiwrapper.h"
#include "../wrapper/perfeventwrapper.h"
#include "../wrapper/raplwrapper.h"
#include "../wrapper/rocmwrapper.h"

namespace meric
{

class Meric {
public:
	static void refreshConfigFromEnv();
	static void readRegionParameters();

	virtual void start(const std::string &region)
	{
		for (unsigned int i = 0; i < _storeList.size(); i++)
		{
			if (_storeList[i]->getMPIWrite())
			{
				_storeList[i]->addStart(getRegionID(region), currentState.socket[0]); // data storing into file with name according socket#0
			}
		}
		MERIC_INFO << "region '" << region << "' started\n";
	}

	virtual double stop(const std::string region = "")
	{
		// stop should be in opposite order as start
		for (int i = _storeList.size() - 1; i >= 0; i--)
		{
			if (_storeList[i]->getMPIWrite())
			{
				_storeList[i]->addEnd(_stack.back());
			}
		}
		MERIC_INFO << "region '" << getRegionName(_stack.back()) << "' stopped\n";
		if (_storeList.size() && _storeList[0]->getMPIWrite())
		{
			// infostore is always the first store in the list
			// only the infostore returns the region runtime, other return 0.0
			return _storeList[0]->getRegionDuration();
		}
		else
		{
			return 0.0;
		}
		}

		virtual void startIgnore()
		{
#ifdef HAVE_OMP_H
			if (omp_in_parallel())
			{
				if (OMPbarriers)
				{
#pragma omp barrier
				}
#pragma omp single nowait
				{
					if (IGNORE)
					{
						std::cerr << "MERIC WARNING: starting nested MERIC ignore sections not supported\n";
					}
					IGNORE = true;
				}
			}
			else
#endif
			{
				if (IGNORE)
				{
					std::cerr << "MERIC WARNING: starting nested MERIC ignore sections not supported\n";
				}
				IGNORE = true;
			}
		}

		virtual void stopIgnore()
		{
#ifdef HAVE_OMP_H
			if (omp_in_parallel())
			{
				if (OMPbarriers)
				{
#pragma omp barrier
				}
#pragma omp single nowait
				{
					if (!IGNORE)
					{
						std::cerr << "MERIC WARNING: stopping already stopped MERIC ignore section\n";
					}
					IGNORE = false;
				}
			}
			else
#endif
			{
				if (!IGNORE)
				{
					std::cerr << "MERIC WARNING: stopping already stopped MERIC ignore section\n";
				}
				IGNORE = false;
			}
		}

		virtual void save()
		{
			MERIC_INFO << "save stored data\n";
			for (unsigned int i = 0; i < _storeList.size(); i++)
			{
				_storeList[i]->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name);
			}
			MERIC_INFO << "store data saved\n";
		}

		virtual void close()
		{
			if (COUNTERS == "perfevent")
			{
				PERFEVENT::close();
			}
			if (COUNTERS == "papi")
			{
				PAPI::close();
			}
			// close() is not needed for RAPL
			if (std::find(MONITORS.begin(), MONITORS.end(), meric::MODE_A64FX) != MONITORS.end())
			{
				meric::A64FX::close();
			}
		}

		virtual ~Meric(){};

	protected:
		static void readRegionOptions();
		static int getRegionSettings(json_object_element_s *data, json_value_s *root, RegionParameters &param);
		static int getDefaultParameters();

		/**
		 * searching for region name in a list of regions that should be ignored
		 **/
		bool ignoreRegion(const std::string &region) { return ignoreRegions.count(region) > 0; }

		/**
		 * current settings to test if there is a change in values, updates the settings
		 * region name
		 * id of a node for the settings is searching
		 **/
		const SocketsRegionParameters regionValue(SocketsRegionParameters &settings, const std::string &region, int node = -1)
		{
			const std::size_t socketCount = std::max(std::size_t(1), static_cast<std::size_t>(Environment::SOCKETS_PER_NODE));
			if (settings.socket.size() < socketCount)
			{
				settings.socket.resize(socketCount);
			}

			SocketsRegionParameters change;
			change.socket.resize(settings.socket.size());
			if (REGION_OPTIONS == "")
			{
				return change;
			}

			auto cmpSettings = [&](SocketsRegionParameters &prev, RegionParameters &next, int socket)
			{
				long paramChange = 0;
				// core frequency
				if (next.frequency)
				{
					paramChange = prev.socket[socket].frequency > next.frequency ? prev.socket[socket].frequency - next.frequency
																				 : next.frequency - prev.socket[socket].frequency;
					if (paramChange > ignoreSettings.frequency)
					{
						change.socket[socket].frequency = paramChange;
						prev.socket[socket].frequency = next.frequency;
					}
				}
				else
				{
					paramChange = prev.socket[socket].frequency > defaultParameter.frequency ? prev.socket[socket].frequency - defaultParameter.frequency
																							 : defaultParameter.frequency - prev.socket[socket].frequency;
					if (paramChange > ignoreSettings.frequency)
					{
						change.socket[socket].frequency = paramChange;
						prev.socket[socket].frequency = defaultParameter.frequency;
					}
				}

				// uncore frequency (Intel, ARM Jetson)
#if defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_GEOPM_H || defined __arm__
				if (next.uncoreFrequency)
				{
					paramChange = prev.socket[socket].uncoreFrequency > next.uncoreFrequency ? prev.socket[socket].uncoreFrequency - next.uncoreFrequency
																							 : next.uncoreFrequency - prev.socket[socket].uncoreFrequency;
					if (paramChange > ignoreSettings.uncoreFrequency)
					{
						change.socket[socket].uncoreFrequency = paramChange;
						prev.socket[socket].uncoreFrequency = next.uncoreFrequency;
					}
				}
				else
				{
					paramChange = prev.socket[socket].uncoreFrequency > defaultParameter.uncoreFrequency
									  ? prev.socket[socket].uncoreFrequency - defaultParameter.uncoreFrequency
									  : defaultParameter.uncoreFrequency - prev.socket[socket].uncoreFrequency;
					if (paramChange > ignoreSettings.uncoreFrequency)
					{
						change.socket[socket].uncoreFrequency = paramChange;
						prev.socket[socket].uncoreFrequency = defaultParameter.uncoreFrequency;
					}
				}
#endif

				// threads
#ifdef HAVE_OMP_H
				if (next.nthreads)
				{
					paramChange = prev.socket[socket].nthreads > next.nthreads ? prev.socket[socket].nthreads - next.nthreads
																			   : next.nthreads - prev.socket[socket].nthreads;
					if (paramChange > ignoreSettings.nthreads)
					{
						change.socket[socket].nthreads = paramChange;
						prev.socket[socket].nthreads = next.nthreads;
					}
				}
				else
				{
					paramChange = prev.socket[socket].nthreads > defaultParameter.nthreads ? prev.socket[socket].nthreads - defaultParameter.nthreads
																						   : defaultParameter.nthreads - prev.socket[socket].nthreads;
					if (paramChange > ignoreSettings.nthreads)
					{
						change.socket[socket].nthreads = paramChange;
						prev.socket[socket].nthreads = defaultParameter.nthreads;
					}
				}
#endif

				// CPU power cap
#if defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_MSR_CORE_H || defined HAVE_GEOPM_H || defined HAVE_E_SMI_E_SMI_H
				if (next.powerCapPKG)
				{
					paramChange = prev.socket[socket].powerCapPKG > next.powerCapPKG ? prev.socket[socket].powerCapPKG - next.powerCapPKG
																					 : next.powerCapPKG - prev.socket[socket].powerCapPKG;
					if (paramChange > ignoreSettings.powerCapPKG)
					{
						change.socket[socket].powerCapPKG = paramChange;
						prev.socket[socket].powerCapPKG = next.powerCapPKG;
					}
				}
				else
				{
					paramChange = prev.socket[socket].powerCapPKG > defaultParameter.powerCapPKG
									  ? prev.socket[socket].powerCapPKG - defaultParameter.powerCapPKG
									  : defaultParameter.powerCapPKG - prev.socket[socket].powerCapPKG;
					if (paramChange > ignoreSettings.powerCapPKG)
					{
						change.socket[socket].powerCapPKG = paramChange;
						prev.socket[socket].powerCapPKG = defaultParameter.powerCapPKG;
					}
				}

				if (next.powerCapPKGtime)
				{
					paramChange = prev.socket[socket].powerCapPKGtime > next.powerCapPKGtime ? prev.socket[socket].powerCapPKGtime - next.powerCapPKGtime
																							 : next.powerCapPKGtime - prev.socket[socket].powerCapPKGtime;
					if (paramChange > ignoreSettings.powerCapPKGtime)
					{
						change.socket[socket].powerCapPKGtime = paramChange;
						prev.socket[socket].powerCapPKGtime = next.powerCapPKGtime;
					}
				}
				else
				{
					paramChange = prev.socket[socket].powerCapPKGtime > defaultParameter.powerCapPKGtime
									  ? prev.socket[socket].powerCapPKGtime - defaultParameter.powerCapPKGtime
									  : defaultParameter.powerCapPKGtime - prev.socket[socket].powerCapPKGtime;
					if (paramChange > ignoreSettings.powerCapPKGtime)
					{
						change.socket[socket].powerCapPKGtime = paramChange;
						prev.socket[socket].powerCapPKGtime = defaultParameter.powerCapPKGtime;
					}
				}
#endif

#if defined HAVE_NVML_H || defined HAVE_ROCM_SMI_ROCM_SMI_H
				// GPU SM frequency
				if (next.frequencyGPU)
				{
					paramChange = prev.socket[socket].frequencyGPU > next.frequencyGPU ? prev.socket[socket].frequencyGPU - next.frequencyGPU
																					   : next.frequencyGPU - prev.socket[socket].frequencyGPU;
					if (paramChange > ignoreSettings.frequencyGPU)
					{
						change.socket[socket].frequencyGPU = paramChange;
						prev.socket[socket].frequencyGPU = next.frequencyGPU;
					}
				}
				else
				{
					paramChange = prev.socket[socket].frequencyGPU > defaultParameter.frequencyGPU
									  ? prev.socket[socket].frequencyGPU - defaultParameter.frequencyGPU
									  : defaultParameter.frequencyGPU - prev.socket[socket].frequencyGPU;
					if (paramChange > ignoreSettings.frequencyGPU)
					{
						change.socket[socket].frequencyGPU = paramChange;
						prev.socket[socket].frequencyGPU = defaultParameter.frequencyGPU;
					}
				}

				// GPU memory frequency
				if (next.frequencyGPUmem)
				{
					paramChange = prev.socket[socket].frequencyGPUmem > next.frequencyGPUmem ? prev.socket[socket].frequencyGPUmem - next.frequencyGPUmem
																							 : next.frequencyGPUmem - prev.socket[socket].frequencyGPUmem;
					if (paramChange > ignoreSettings.frequencyGPUmem)
					{
						change.socket[socket].frequencyGPUmem = paramChange;
						prev.socket[socket].frequencyGPUmem = next.frequencyGPUmem;
					}
				}
				else
				{
					paramChange = prev.socket[socket].frequencyGPUmem > defaultParameter.frequencyGPUmem
									  ? prev.socket[socket].frequencyGPUmem - defaultParameter.frequencyGPUmem
									  : defaultParameter.frequencyGPUmem - prev.socket[socket].frequencyGPUmem;
					if (paramChange > ignoreSettings.frequencyGPUmem)
					{
						change.socket[socket].frequencyGPUmem = paramChange;
						prev.socket[socket].frequencyGPUmem = defaultParameter.frequencyGPUmem;
					}
				}

				// GPU power cap
				if (next.powerCapGPU)
				{
					paramChange = prev.socket[socket].powerCapGPU > next.powerCapGPU ? prev.socket[socket].powerCapGPU - next.powerCapGPU
																					 : next.powerCapGPU - prev.socket[socket].powerCapGPU;
					if (paramChange > ignoreSettings.powerCapGPU)
					{
						change.socket[socket].powerCapGPU = paramChange;
						prev.socket[socket].powerCapGPU = next.powerCapGPU;
					}
				}
				else
				{
					paramChange = prev.socket[socket].powerCapGPU > defaultParameter.powerCapGPU
									  ? prev.socket[socket].powerCapGPU - defaultParameter.powerCapGPU
									  : defaultParameter.powerCapGPU - prev.socket[socket].powerCapGPU;
					if (paramChange > ignoreSettings.powerCapGPU)
					{
						change.socket[socket].powerCapGPU = paramChange;
						prev.socket[socket].powerCapGPU = defaultParameter.powerCapGPU;
					}
				}
#endif
			};

			if (parameter.find(region) == parameter.end()) // region name not in list
			{
				for (int socket = 0; socket < Environment::SOCKETS_PER_NODE; socket++)
				{
					cmpSettings(settings, defaultParameter, socket);
				}
			}
			else
			{
				std::map<std::pair<int, int>, struct RegionParameters>::iterator it;
				for (int socket = 0; socket < Environment::SOCKETS_PER_NODE; socket++)
				{
					if (node != -1 && (it = parameter[region].find(std::make_pair(node, socket))) != parameter[region].end())
					{
						cmpSettings(settings, it->second, socket);
					}
					else if (node != -1 && (it = parameter[region].find(std::make_pair(node, -1))) != parameter[region].end())
					{
						cmpSettings(settings, it->second, socket);
					}
					else if ((it = parameter[region].find(std::make_pair(-1, socket))) != parameter[region].end())
					{
						cmpSettings(settings, it->second, socket);
					}
					else if ((it = parameter[region].find(std::make_pair(-1, -1))) != parameter[region].end())
					{
						cmpSettings(settings, it->second, socket);
					}
					else
					{
						cmpSettings(settings, defaultParameter, socket);
					}
				}
			}
			/*
					MERIC_INFO << region << " CHANGE\n"
						<< "\tFRQ "<< change.socket[0].frequency << std::endl
						<< "\tunF "<< change.socket[0].uncoreFrequency << std::endl
						<< "\tTHR "<< change.socket[0].nthreads << std::endl
						<< "\tPWR "<< change.socket[0].powerCapPKG << std::endl
						<< "\tTWD "<< change.socket[0].powerCapPKGtime << std::endl
						;
			*/
			return change;
		}

		/**
		 * Function sets the environment settings at the region start/stop
		 * change - change size in each parameter
		 * MPISelection - frequencies are set by one process per node
		 **/
		void setRegionEnvironment(SocketsRegionParameters change, bool MPIselection = true)
		{
			if (MPIselection)
			{
				// set core frequency
				bool differentSettingsForSockets = false;
				long diffCore = change.socket[0].frequency;
				for (int socket = 1; socket < Environment::SOCKETS_PER_NODE; socket++)
				{
					if (diffCore != change.socket[socket].frequency)
					{
						differentSettingsForSockets = true;
					}
				}

				if (differentSettingsForSockets)
				{
					for (int socket = 0; socket < Environment::SOCKETS_PER_NODE; socket++)
					{
						if (change.socket[socket].frequency)
						{
							Environment::frequency(currentState.socket[socket].frequency, socket);
						}
					}
				}
				else if (diffCore)
				{
					Environment::frequency(currentState.socket[0].frequency);
				}

				// set uncore frequncy
				differentSettingsForSockets = false;
				long diffUncore = change.socket[0].uncoreFrequency;
				for (int socket = 1; socket < Environment::SOCKETS_PER_NODE; socket++)
				{
					if (diffUncore != change.socket[socket].uncoreFrequency)
					{
						differentSettingsForSockets = true;
					}
				}

				if (differentSettingsForSockets)
				{
					for (int socket = 0; socket < Environment::SOCKETS_PER_NODE; socket++)
					{
						if (change.socket[socket].uncoreFrequency)
						{
							Environment::uncoreFrequency(currentState.socket[socket].uncoreFrequency, currentState.socket[socket].uncoreFrequency, socket);
						}
					}
				}
				else if (diffUncore)
				{
					Environment::uncoreFrequency(currentState.socket[0].uncoreFrequency, currentState.socket[0].uncoreFrequency);
				}

				// set powercap - to change timeWindow also power must be different
				differentSettingsForSockets = false;
				long diffCapPower = change.socket[0].powerCapPKG;
				long diffCapTime = change.socket[0].powerCapPKGtime;
				for (int socket = 1; socket < Environment::SOCKETS_PER_NODE; socket++)
				{
					if (diffCapPower != change.socket[socket].powerCapPKG || diffCapTime != change.socket[0].powerCapPKGtime)
					{
						differentSettingsForSockets = true;
						break;
					}
				}

				if (differentSettingsForSockets)
				{
					for (int socket = 0; socket < Environment::SOCKETS_PER_NODE; socket++)
					{
						if (change.socket[socket].powerCapPKG)
						{
							Environment::CPUpowercap(currentState.socket[socket].powerCapPKG, currentState.socket[socket].powerCapPKGtime, socket);
						}
					}
				}
				else if (diffCapPower)
				{
					Environment::CPUpowercap(currentState.socket[0].powerCapPKG, currentState.socket[0].powerCapPKGtime);
				}
			}
			if (change.socket[0].nthreads)
			{
				Environment::threadNumber(currentState.socket[0].nthreads);
			}
		}

		bool validateRegionName(const std::string &region)
		{
			bool valid = true;
			const size_t pos = find(_name.begin(), _name.end(), region) - _name.begin();
			if (pos >= _name.size()) // 1st time executed region
			{
				const int len = region.length();
				// contains invalid characters
				for (const auto &s : {'/', '\\', ':', '*', '?', '"', '<', '>', '|', '@', '#'})
				{
					if (region.find_first_of(s) != std::string::npos)
					{
						valid = false;
						break;
					}
				}
				// too long name or reserved name
				if (valid && (len > 255 || region == "." || region == ".."))
				{
					valid = false;
				}
				// ends with _static but it is not main application region
				if (valid && _stack.size() != 0 && len > 7 && region.substr(len - 7, 7) == "_static")
				{
					valid = false;
				}
				if (valid == false)
				{
#pragma omp single nowait
					{
						ignoreRegions.insert(region);
						std::cerr << "MERIC WARNING: invalid region name '" << region << "' - region ignored" << std::endl;
					}
				}
			}
			return valid;
		}

		const std::string &getRegionName(int regionId)
		{
			if (regionId < 0 || regionId >= _name.size())
				return _mainRegion;

			return _name[regionId];
		}

		const std::string &getParentRegionName(bool skipIgnored = true)
		{
			if (_stack.size() <= 1)
				return _mainRegion;

			for (auto it = std::next(_stack.crbegin()); it != _stack.crend(); ++it)
			{
				const std::string &name = getRegionName(*it);

				if (skipIgnored && ignoreRegion(name))
					continue;

				return name;
			}

			return _mainRegion;
		}

		int getRegionID(const std::string &region)
		{
			size_t pos = find(_name.begin(), _name.end(), region) - _name.begin();
			if (pos >= _name.size())
			{
				pos = _name.size();
				_name.push_back(region);
			}
			return pos;
		}

		std::vector<std::string> get_cmdline_args_linux()
		{
			std::ifstream f("/proc/self/cmdline", std::ios::binary);
			std::string buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

			std::vector<std::string> args;
			size_t i = 0;
			while (i < buf.size())
			{
				size_t j = buf.find('\0', i);
				if (j == std::string::npos)
				{
					break;
				}
				if (j > i)
				{
					args.emplace_back(buf.data() + i, j - i);
				}
				i = j + 1;
			}
			return args;
		}

		// Sets static region name according to the application name. Skips interpreter name if detected
		void setMainRegionName()
		{
			_mainRegion.assign(program_invocation_name);
			if (_mainRegion.find("python") == 0)
			{
				const auto args = get_cmdline_args_linux();
				if (args.size() > 1)
				{
					std::string candidate = args[1];
					size_t n = candidate.find_last_of("/");
					if (n != std::string::npos)
					{
						candidate = candidate.substr(n + 1);
					}
					n = candidate.find_first_of(".");
					if (n != std::string::npos)
					{
						candidate = candidate.substr(0, n);
					}
					const bool ok = !candidate.empty() && std::all_of(candidate.begin(), candidate.end(),
																	  [](char c)
																	  {
																		  const unsigned char uc = static_cast<unsigned char>(c);
																		  return std::isalnum(uc) || c == '_' || c == '-';
																	  });
					if (ok)
					{
						_mainRegion = candidate;
					}
				}
			}

			size_t n = _mainRegion.find_last_of("/");
			if (n != std::string::npos)
			{
				_mainRegion = _mainRegion.substr(n + 1);
			}
			n = _mainRegion.find_first_of(".");
			if (n != std::string::npos)
			{
				_mainRegion = _mainRegion.substr(0, n);
			}
			_mainRegion += "_static";
		}

		//              region_name                     node socket       settings
		static std::map<std::string, std::map<std::pair<int, int>, struct RegionParameters>> parameter;
		static RegionParameters ignoreSettings;
		static std::set<std::string> ignoreRegions;

		static RegionParameters defaultParameter;
		static SocketsRegionParameters currentState;

		std::vector<int> _stack;
		std::vector<std::string> _name;
		std::vector<StoreBase *> _storeList;
		std::string _mainRegion;
		static timespec _startTime, _stopTime;
		static bool MPIbarriers;
		static bool OMPbarriers;
		static std::string outputFilenameInfo;
		static RegionParameters outputFormat;
		static double timeThreshold;

		static bool DETAILED;
		static bool AGGREGATE;
		static bool DEBUG;
		static bool SAMPLES;
		static double SAMPLING_FREQUENCY;
		static bool IGNORE;
		static bool HDF5;
		static int MODE;
		static std::string MERIC_E4MACHINE_TO_MONITOR;
		static std::string COUNTERS;
		static std::string OUTPUT_DIR;
		static std::string OUTPUT_FILENAME;
		static std::string REGION_OPTIONS;
		static std::string BARRIERS;
		static std::string MEASURE;
		static std::vector<mode> MONITORS;
		static std::string THREAD_PIN_MODE;

		std::unique_ptr<meric::cpu_affinity_map::CPUAffinityMap_t> m_pAffinityMap;
};

} // namespace meric

#endif /* SRC_MERIC_MERIC_H_ */
