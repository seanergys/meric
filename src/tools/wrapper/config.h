
#ifndef SRC_TOOLS_WRAPPER_CONFIG_H_
#define SRC_TOOLS_WRAPPER_CONFIG_H_

#include <map>

#include "../../wrapper/environmentwrapper.h"

//namespace wrapper{

struct CFGbase {
	virtual ~CFGbase() = default;
	std::map<std::string, CFGbase*> config;
	virtual bool lastLevel() = 0;
	virtual void setValue(std::string s) = 0;
};

struct CFGtunedParameter : public CFGbase {
//	virtual bool lastLevel() = 0;
//	virtual void setValue(std::string s) = 0;
	virtual void fillList() = 0;
};



//template <long>
struct CFGbase_list : public CFGbase {
	const std::string parameterName;
	bool lastLevel() { return true; }
	std::vector<long> value;

	CFGbase_list() : parameterName("LIST") {};
	void setValue(std::string s)
	{
		value.push_back(std::stol(s));
//std::cout << parameterName <<"[" <<value.size()-1<< "] : " << value.back() << std::endl;
	}
};

struct CFG_mpi : public CFGbase {
	const std::string parameterName;
	bool value;
	bool lastLevel() { return true; }

	CFG_mpi() : parameterName("MPI"), value(false) {};
	void setValue(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), ::toupper);
		if (s == "TRUE")
			value = true;
//std::cout << parameterName << " : " << value << std::endl;
	}

};

struct CFG_baseline : public CFGbase {
	const std::string parameterName;
	double value;
	bool lastLevel() { return true; }
	
	CFG_baseline() : parameterName("BASELINE"), value(0.0) {};
	
	void setValue(std::string s)
	{
		value = std::stod(s);
//std::cout << parameterName << " : " << value << std::endl;
	}
};

struct CFG_margin : public CFGbase {
	const std::string parameterName;
	double value;
	bool lastLevel() { return true; }
	
	CFG_margin() : parameterName("MARGIN"), value(1.0) {};
	
	void setValue(std::string s)
	{
		value = 1.0 + std::stod(s)/100.0;
//std::cout << parameterName << " : " << value << std::endl;
	}
};

struct CFG_extention : public CFGbase {
	const std::string parameterName;
	double value;
	bool lastLevel() { return true; }
	
	CFG_extention() : parameterName("EXTENTION"), value(1.1) {};
	
	void setValue(std::string s)
	{
		value = 1.0 + std::stod(s)/100.0;
//std::cout << parameterName << " : " << value << std::endl;
	}
};

	struct CFG_algorithm_name : public  CFGbase {
		const std::string parameterName;
		std::string value;
		bool lastLevel() { return true; }
		std::vector <std::string> supportedValues;
		
		CFG_algorithm_name() : parameterName("NAME"), value ("EXHAUSTIVE") 
		{
			supportedValues.push_back("EXHAUSTIVE");
			supportedValues.push_back("EVO");
			supportedValues.push_back("PSO");
				supportedValues.push_back("RANDOM");
			supportedValues.push_back("NONE");
		};
		
		void setValue(std::string s) //TODO bool
		{
			if (std::find(supportedValues.begin(), supportedValues.end(), s) != supportedValues.end())
				value = s;
			else
				std::cerr << "UNSUPPORTED PARAMETER VALUE "<< s << std::endl;
//std::cout << parameterName << " : " << value << std::endl;
		}
	};
	
		struct CFG_algorithm_parameters_condition : public CFGbase {
			const std::string parameterName;
			int value;
			bool lastLevel() { return true; }

			CFG_algorithm_parameters_condition() : parameterName("END_CONDITION"), value(2) {};
			void setValue(std::string s)
			{
				value = std::stoi(s);
//std::cout << parameterName << " : " << value << std::endl;
			}
		};
		
		struct CFG_algorithm_parameters_population : public CFGbase {
			const std::string parameterName;
			int value;
			bool lastLevel() { return true; }

			CFG_algorithm_parameters_population() : parameterName("POPULATION"), value(10) {};
			void setValue(std::string s)
			{
				value = std::stoi(s);
//std::cout << parameterName << " : " << value << std::endl;
			}
		};
		
		struct CFG_algorithm_parameters_particles : public CFGbase {
			const std::string parameterName;
			int value;
			bool lastLevel() { return true; }

			CFG_algorithm_parameters_particles() : parameterName("PARTICLES"), value(10) {};
			void setValue(std::string s)
			{
				value = std::stoi(s);
//std::cout << parameterName << " : " << value << std::endl;
			}
		};
		
		struct CFG_algorithm_parameters_cp : public CFGbase {
			const std::string parameterName;
			double value;
			bool lastLevel() { return true; }

			CFG_algorithm_parameters_cp() : parameterName("CONST_CP"), value(2.05) {};
			void setValue(std::string s)
			{
				value = std::stod(s);
//std::cout << parameterName << " : " << value << std::endl;
			}
		};
		
		struct CFG_algorithm_parameters_cg : public CFGbase {
			const std::string parameterName;
			double value;
			bool lastLevel() { return true; }

			CFG_algorithm_parameters_cg() : parameterName("CONST_CG"), value(2.05) {};
			void setValue(std::string s)
			{
				value = std::stod(s);
//std::cout << parameterName << " : " << value << std::endl;
			}
		};

		struct CFG_algorithm_parameters_w : public CFGbase {
			const std::string parameterName;
			double value;
			bool lastLevel() { return true; }

			CFG_algorithm_parameters_w() : parameterName("CONST_W"), value(0.9) {};
			void setValue(std::string s)
			{
				value = std::stod(s);
//std::cout << parameterName << " : " << value << std::endl;
			}
		};
		
	struct CFG_algorithm_parameters : public CFGbase {
		const std::string parameterName;
		bool lastLevel() { return false; }
		
		CFG_algorithm_parameters_condition condition;
		CFG_algorithm_parameters_population population;
		CFG_algorithm_parameters_particles particles;
		CFG_algorithm_parameters_cp cp;
		CFG_algorithm_parameters_cg cg;
		CFG_algorithm_parameters_w w;
		
		
		CFG_algorithm_parameters() : parameterName("PARAMETERS")
		{
			config.insert(std::pair<std::string,CFGbase*>(condition.parameterName,&condition));
			config.insert(std::pair<std::string,CFGbase*>(population.parameterName,&population));
			config.insert(std::pair<std::string,CFGbase*>(particles.parameterName,&particles));
			config.insert(std::pair<std::string,CFGbase*>(cp.parameterName,&cp));
			config.insert(std::pair<std::string,CFGbase*>(cg.parameterName,&cg));
			config.insert(std::pair<std::string,CFGbase*>(w.parameterName,&w));
		};
		void setValue(std::string s) {}
	};

struct CFG_algorithm : public CFGbase {
	const std::string parameterName;
	CFG_algorithm_parameters parameters;
	CFG_algorithm_name name;
	bool lastLevel() { return false; }

	CFG_algorithm() : parameterName("ALGORITHM")
	{
		config.insert(std::pair<std::string,CFGbase*>(name.parameterName,&name));
		config.insert(std::pair<std::string,CFGbase*>(parameters.parameterName,&parameters));
	}
	void setValue(std::string s) {}
};

	struct CFG_meric_measure : public CFGbase { //TODO currently only one measurement system at one time is supported
		const std::string parameterName;
		bool lastLevel() { return true; }
		meric::mode value;
		
		CFG_meric_measure() : parameterName("MEASURE"),
#ifdef HAVE_HDEEM_H
		value(meric::MODE_HDEEM_SAMPLES)
#elif defined HAVE_X86_ADAPT_H || defined HAVE_MSR_SAFE || defined HAVE_GEOPM_H
		value(meric::MODE_RAPL)
#elif defined HAVE_NVML_H
		value(meric::MODE_NVML)
#elif defined HAVE_E4_H
			  value(meric::MODE_PDU)
#elif defined HAVE_OCC
		value(meric::MODE_OCC)
#elif defined HAVE_RESTCLIENT_CPP_RESTCLIENT_H
		value(meric::MODE_DAVIDE)
#else // just time measurement
		value(meric::MODE_TIME)
#endif
		{};
		void setValue(std::string s)
		{
			if (meric::modeString.count(s) == 0)
			{
				std::cerr << "Invalid measurement system selected, currently only one at the time is supported.\n";
			}
			else
				value = meric::modeString[s];
//std::cout << parameterName << " : " << (int) value << std::endl;
		}
		
		std::string getValue()
		{
			for (auto &i : meric::modeString)
			{
				if (i.second == value)
				{
					return i.first;
				}
			}
		}
	};

	struct CFG_meric_aggregate : public CFGbase {
		const std::string parameterName;
		bool lastLevel() { return true; }
		int value;
		
		CFG_meric_aggregate() : parameterName("AGGREGATE"), value(1) {};
		void setValue(std::string s)
		{
			value = std::stoi(s);
//std::cout << parameterName << " : " << value << std::endl;
		}
	};
	
	struct CFG_meric_detailed : public CFGbase {
		const std::string parameterName;
		bool lastLevel() { return true; }
		int value;
		
		CFG_meric_detailed() : parameterName("DETAILED"), value(1) {};
		void setValue(std::string s)
		{
			value = std::stoi(s);
//std::cout << parameterName << " : " << value << std::endl;
		}
	};
	
	struct CFG_meric_samples : public CFGbase {
		const std::string parameterName;
		bool lastLevel() { return true; }
		int value;
		
		CFG_meric_samples() : parameterName("SAMPLES"), value(0) {};
		void setValue(std::string s)
		{
			value = std::stoi(s);
//std::cout << parameterName << " : " << value << std::endl;
		}
	};
	
	struct CFG_meric_barriers : public CFGbase {
		const std::string parameterName;
		bool lastLevel() { return true; }
		std::string value;
		std::vector <std::string> supportedValues;
		
		CFG_meric_barriers() : parameterName("BARRIERS"), value("all") 
		{
			supportedValues.push_back("all");
			supportedValues.push_back("mpi");
			supportedValues.push_back("omp");
			supportedValues.push_back("none");
		};
		void setValue(std::string s)
		{
			if (std::find(supportedValues.begin(), supportedValues.end(), s) != supportedValues.end())
				value = s;
			else
				std::cerr << "UNSUPPORTED PARAMETER VALUE "<< s << std::endl;
//std::cout << parameterName << " : " << value << std::endl;
		}
	};
	
	struct CFG_meric_output : public CFGbase {
		const std::string parameterName;
		bool lastLevel() { return true; }
		std::string value;
		
		CFG_meric_output() : parameterName("OUTPUT_DIR"), value("") {};
		void setValue(std::string s)
		{
			value = s;
//std::cout << parameterName << " : " << value << std::endl;
		}
	};
	
	struct CFG_meric_counters : public CFGbase {
		const std::string parameterName;
		bool lastLevel() { return true; }
		std::string value;
		std::vector <std::string> supportedValues;
		
		CFG_meric_counters() : parameterName("COUNTERS"), 
#ifdef HAVE_MSR_SAFE
		value("msr")
#elif defined HAVE_PAPI_H
		value("papi")
#elif defined HAVE_PERFEVENT_H
		value("perfevent")
#else
		value("")
#endif		
		{
			supportedValues.push_back("msr");
			supportedValues.push_back("papi");
			supportedValues.push_back("perfevent");
		};
		void setValue(std::string s)
		{
			if (std::find(supportedValues.begin(), supportedValues.end(), s) != supportedValues.end())
				value = s;
			else
				std::cerr << "UNSUPPORTED PARAMETER VALUE "<< s << std::endl;
//std::cout << parameterName << " : " << value << std::endl;
		}
	};
	
	
struct CFG_meric : public CFGbase {
	const std::string parameterName;
	bool lastLevel() { return false; }

	//CF, UCF, THRDS, CAP_POWER, CAP_TIME ?
	CFG_meric_measure measure;
	CFG_meric_aggregate aggregate;
	CFG_meric_detailed detailed;
	CFG_meric_samples samples;
	CFG_meric_barriers barriers;
	CFG_meric_output outputdir;
	CFG_meric_counters counters;

	CFG_meric() : parameterName("MERIC")
	{
		config.insert(std::pair<std::string,CFGbase*>(measure.parameterName,&measure));
		config.insert(std::pair<std::string,CFGbase*>(aggregate.parameterName,&aggregate));
		config.insert(std::pair<std::string,CFGbase*>(detailed.parameterName,&detailed));
		config.insert(std::pair<std::string,CFGbase*>(samples.parameterName,&samples));
		config.insert(std::pair<std::string,CFGbase*>(barriers.parameterName,&barriers));
		config.insert(std::pair<std::string,CFGbase*>(outputdir.parameterName,&outputdir));
#if defined HAVE_MSR_SAFE || defined HAVE_PAPI_H || defined HAVE_PERFEVENT_H
		config.insert(std::pair<std::string,CFGbase*>(counters.parameterName,&counters));
#endif
	}
	void setValue(std::string s) {}
};

		struct CFG_parameters_param_max : public CFGbase {
			const std::string parameterName;
			bool lastLevel() { return true; }
			long value;
			
			CFG_parameters_param_max() : parameterName("MAX"), value(0) {};
			void setValue(std::string s)
			{
				value = std::stol(s);
	//std::cout << parameterName << " : " << value << std::endl;
			}
		};
		
		struct CFG_parameters_param_min : public CFGbase {
			const std::string parameterName;
			bool lastLevel() { return true; }
			long value;
			
			CFG_parameters_param_min() : parameterName("MIN"), value(0) {};
			void setValue(std::string s)
			{
				value = std::stol(s);
	//std::cout << parameterName << " : " << value << std::endl;
			}
		};
		
		struct CFG_parameters_param_step : public CFGbase {
			const std::string parameterName;
			bool lastLevel() { return true; }
			long value;
			
			CFG_parameters_param_step() : parameterName("STEP"), value(1) {};
			void setValue(std::string s)
			{
				value = std::stol(s);
	//std::cout << parameterName << " : " << value << std::endl;
			}
		};
		

//	struct CFG_parameters_frequency : public CFGbase {
	struct CFG_parameters_frequency : public CFGtunedParameter {
		const std::string parameterName;
		bool lastLevel() { return false; }
		CFG_parameters_param_max max;
		CFG_parameters_param_min min;
		CFG_parameters_param_step step;
		CFGbase_list list;
		
		void fillList()
		{
			if (list.value.size() == 0 && step.value > 0)
			{
				for (auto c = min.value; c < max.value; c+= step.value)
				{
					list.value.push_back(c);
				}
				if (max.value != 0)
					list.value.push_back(max.value);
			}
			//std::cout << parameterName <<" list: ";
			//for (auto x : list.value)
			//	std::cout << x << " ";
			//std::cout << std::endl;
		}
		
		CFG_parameters_frequency() : parameterName("FREQUENCY")
		{
			config.insert(std::pair<std::string,CFGbase*>(max.parameterName,&max));
			config.insert(std::pair<std::string,CFGbase*>(min.parameterName,&min));
			config.insert(std::pair<std::string,CFGbase*>(step.parameterName,&step));
			config.insert(std::pair<std::string,CFGbase*>(list.parameterName,&list));
		};
		void setValue(std::string s) {}
	};

//	struct CFG_parameters_uncorefrequency : public CFGbase {
	struct CFG_parameters_uncorefrequency : public CFGtunedParameter {
		const std::string parameterName;
		bool lastLevel() { return false; }
		CFG_parameters_param_max max;
		CFG_parameters_param_min min;
		CFG_parameters_param_step step;
		CFGbase_list list;
		
		void fillList()
		{
			if (list.value.size() == 0 && step.value > 0)
			{
				for (auto c = min.value; c < max.value; c+= step.value)
				{
					list.value.push_back(c);
				}
				if (max.value != 0)
					list.value.push_back(max.value);
			}
			//std::cout << parameterName <<" list: ";
			//for (auto x : list.value)
			//	std::cout << x << " ";
			//std::cout << std::endl;
		}
		CFG_parameters_uncorefrequency() : parameterName("UNCORE_FREQUENCY")
		{
			config.insert(std::pair<std::string,CFGbase*>(max.parameterName,&max));
			config.insert(std::pair<std::string,CFGbase*>(min.parameterName,&min));
			config.insert(std::pair<std::string,CFGbase*>(step.parameterName,&step));
			config.insert(std::pair<std::string,CFGbase*>(list.parameterName,&list));
		};
		void setValue(std::string s) {}
	};

//	struct CFG_parameters_threads : public CFGbase {
	struct CFG_parameters_threads : public CFGtunedParameter {
		const std::string parameterName;
		bool lastLevel() { return false; }
		CFG_parameters_param_max max;
		CFG_parameters_param_min min;
		CFG_parameters_param_step step;
		CFGbase_list list;
		
		void fillList()
		{
			if (list.value.size() == 0 && step.value > 0)
			{
				for (auto c = min.value; c < max.value; c+= step.value)
				{
					list.value.push_back(c);
				}
				if (max.value != 0)
					list.value.push_back(max.value);
			}
		}
		CFG_parameters_threads() : parameterName("NUM_THREADS")
		{
			config.insert(std::pair<std::string,CFGbase*>(max.parameterName,&max));
			config.insert(std::pair<std::string,CFGbase*>(min.parameterName,&min));
			config.insert(std::pair<std::string,CFGbase*>(step.parameterName,&step));
			config.insert(std::pair<std::string,CFGbase*>(list.parameterName,&list));
		};
		void setValue(std::string s) {}
	};
	
//	struct CFG_parameters_capPower : public CFGbase {
	struct CFG_parameters_capPower : public CFGtunedParameter {
		const std::string parameterName;
		bool lastLevel() { return false; }
		CFG_parameters_param_max max;
		CFG_parameters_param_min min;
		CFG_parameters_param_step step;
		CFGbase_list list;
		
		void fillList()
		{
			if (list.value.size() == 0 && step.value > 0)
			{
				for (auto c = min.value; c < max.value; c+= step.value)
				{
					list.value.push_back(c);
				}
				if (max.value != 0)
					list.value.push_back(max.value);
			}
		}

		CFG_parameters_capPower() : parameterName("PWRCAP_POWER")
		{
			config.insert(std::pair<std::string,CFGbase*>(max.parameterName,&max));
			config.insert(std::pair<std::string,CFGbase*>(min.parameterName,&min));
			config.insert(std::pair<std::string,CFGbase*>(step.parameterName,&step));
			config.insert(std::pair<std::string,CFGbase*>(list.parameterName,&list));
		};
		void setValue(std::string s) {}
	};

//	struct CFG_parameters_capTime : public CFGbase {
	struct CFG_parameters_capTime : public CFGtunedParameter {
		const std::string parameterName;
		bool lastLevel() { return false; }
		CFG_parameters_param_max max;
		CFG_parameters_param_min min;
		CFG_parameters_param_step step;
		CFGbase_list list;
		
		void fillList()
		{
			if (list.value.size() == 0 && step.value > 0)
			{
				for (auto c = min.value; c < max.value; c+= step.value)
				{
					list.value.push_back(c);
				}
				if (max.value != 0)
					list.value.push_back(max.value);
			}
		}
		CFG_parameters_capTime() : parameterName("PWRCAP_TIME")
		{
			config.insert(std::pair<std::string,CFGbase*>(max.parameterName,&max));
			config.insert(std::pair<std::string,CFGbase*>(min.parameterName,&min));
			config.insert(std::pair<std::string,CFGbase*>(step.parameterName,&step));
			config.insert(std::pair<std::string,CFGbase*>(list.parameterName,&list));
		};
		void setValue(std::string s) {}
	};
	
	struct CFG_parameters_GPUfrequency : public CFGtunedParameter {
		const std::string parameterName;
		bool lastLevel() { return false; }
		CFG_parameters_param_max max;
		CFG_parameters_param_min min;
		CFG_parameters_param_step step;
		CFGbase_list list;
		
		void fillList()
		{
			if (list.value.size() == 0 && step.value > 0)
			{
				for (auto c = min.value; c < max.value; c+= step.value)
				{
					list.value.push_back(c);
				}
				if (max.value != 0)
					list.value.push_back(max.value);
			}
			//std::cout << parameterName <<" list: ";
			//for (auto x : list.value)
			//	std::cout << x << " ";
			//std::cout << std::endl;
		}
		
		CFG_parameters_GPUfrequency() : parameterName("GPU_FREQUENCY")
		{
			config.insert(std::pair<std::string,CFGbase*>(max.parameterName,&max));
			config.insert(std::pair<std::string,CFGbase*>(min.parameterName,&min));
			config.insert(std::pair<std::string,CFGbase*>(step.parameterName,&step));
			config.insert(std::pair<std::string,CFGbase*>(list.parameterName,&list));
		};
		void setValue(std::string s) {}
	};

	struct CFG_parameters_GPUMEMfrequency : public CFGtunedParameter {
		const std::string parameterName;
		bool lastLevel() { return false; }
		CFG_parameters_param_max max;
		CFG_parameters_param_min min;
		CFG_parameters_param_step step;
		CFGbase_list list;
		
		void fillList()
		{
			if (list.value.size() == 0 && step.value > 0)
			{
				for (auto c = min.value; c < max.value; c+= step.value)
				{
					list.value.push_back(c);
				}
				if (max.value != 0)
					list.value.push_back(max.value);
			}
			//std::cout << parameterName <<" list: ";
			//for (auto x : list.value)
			//	std::cout << x << " ";
			//std::cout << std::endl;
		}
		
		CFG_parameters_GPUMEMfrequency() : parameterName("GPU_MEM_FREQUENCY")
		{
			config.insert(std::pair<std::string,CFGbase*>(max.parameterName,&max));
			config.insert(std::pair<std::string,CFGbase*>(min.parameterName,&min));
			config.insert(std::pair<std::string,CFGbase*>(step.parameterName,&step));
			config.insert(std::pair<std::string,CFGbase*>(list.parameterName,&list));
		};
		void setValue(std::string s) {}
	};
	
	struct CFG_parameters_capPowerGPU : public CFGtunedParameter {
		const std::string parameterName;
		bool lastLevel() { return false; }
		CFG_parameters_param_max max;
		CFG_parameters_param_min min;
		CFG_parameters_param_step step;
		CFGbase_list list;
		
		void fillList()
		{
			if (list.value.size() == 0 && step.value > 0)
			{
				for (auto c = min.value; c < max.value; c+= step.value)
				{
					list.value.push_back(c);
				}
				if (max.value != 0)
					list.value.push_back(max.value);
			}
		}
		CFG_parameters_capPowerGPU() : parameterName("PWRCAP_GPU")
		{
			config.insert(std::pair<std::string,CFGbase*>(max.parameterName,&max));
			config.insert(std::pair<std::string,CFGbase*>(min.parameterName,&min));
			config.insert(std::pair<std::string,CFGbase*>(step.parameterName,&step));
			config.insert(std::pair<std::string,CFGbase*>(list.parameterName,&list));
		};
		void setValue(std::string s) {}
	};

	
struct CFG_parameters : public CFGbase {
	const std::string parameterName;
	bool lastLevel() { return false; }

	CFG_parameters_frequency frequency;
	CFG_parameters_uncorefrequency uncorefrequency;
	CFG_parameters_threads threads;
	CFG_parameters_capPower capPower;
	CFG_parameters_capTime capTime;
	
	CFG_parameters_GPUfrequency GPUSMfrequency;
	CFG_parameters_GPUMEMfrequency GPUMEMfrequency;
	CFG_parameters_capPowerGPU GPUcapPower;

	CFG_parameters() : parameterName("PARAMETERS")
	{
/*		config.insert(std::pair<std::string,CFGbase*>(frequency.parameterName,&frequency));
		config.insert(std::pair<std::string,CFGbase*>(uncorefrequency.parameterName,&uncorefrequency));
		config.insert(std::pair<std::string,CFGbase*>(threads.parameterName,&threads));
		config.insert(std::pair<std::string,CFGbase*>(capPower.parameterName,&capPower));
		config.insert(std::pair<std::string,CFGbase*>(capTime.parameterName,&capTime));
*/
		config.insert(std::pair<std::string,CFGtunedParameter*>(frequency.parameterName,&frequency));
		config.insert(std::pair<std::string,CFGtunedParameter*>(uncorefrequency.parameterName,&uncorefrequency));
		config.insert(std::pair<std::string,CFGtunedParameter*>(threads.parameterName,&threads));
		config.insert(std::pair<std::string,CFGtunedParameter*>(capPower.parameterName,&capPower));
		config.insert(std::pair<std::string,CFGtunedParameter*>(capTime.parameterName,&capTime));
		config.insert(std::pair<std::string,CFGtunedParameter*>(GPUSMfrequency.parameterName,&GPUSMfrequency));
		config.insert(std::pair<std::string,CFGtunedParameter*>(GPUMEMfrequency.parameterName,&GPUMEMfrequency));
		config.insert(std::pair<std::string,CFGtunedParameter*>(GPUcapPower.parameterName,&GPUcapPower));
	}
	void setValue(std::string s) {}
};

struct cfg : public CFGbase {

	CFG_mpi mpi;
	CFG_baseline baseline;
	CFG_margin margin;
	CFG_extention extention;
	CFG_algorithm algorithm;
	CFG_parameters parameters;
	CFG_meric meric;
	std::string application;

	bool lastLevel() { return false; }
	void setValue(std::string s) {}
	
	cfg() 
	{
		config.insert(std::pair<std::string,CFGbase*>(mpi.parameterName,&mpi));
		config.insert(std::pair<std::string,CFGbase*>(baseline.parameterName,&baseline));
		config.insert(std::pair<std::string,CFGbase*>(algorithm.parameterName,&algorithm));
		config.insert(std::pair<std::string,CFGbase*>(parameters.parameterName,&parameters));
		config.insert(std::pair<std::string,CFGbase*>(meric.parameterName,&meric));
	}
	
	std::string getOutputFormat()
	{
		//TODO node
		std::string format = "";
		/* TODO
		if (meric.outputFormat.value != "")
			return meric.outputFormat.value;
		else
		*/
		if (parameters.frequency.list.value.size())
			format = "CF";
		if (parameters.uncorefrequency.list.value.size())
		{
			if (!format.empty())
				format += "_";
			format += "UnCF";
		}
		if (parameters.threads.list.value.size())
		{
			if (!format.empty())
				format += "_";
			format += "thrds";
		}
		if (parameters.capPower.list.value.size())
		{
			if (!format.empty())
				format += "_";
			format += "CPUPWR";
		}
		if (parameters.capTime.list.value.size())
		{
			if (!format.empty())
				format += "_";
			format += "CPUPWRtw";
		}
		if (parameters.GPUSMfrequency.list.value.size())
		{
			if (!format.empty())
				format += "_";
			format += "GPUF";
		}
		if (parameters.GPUMEMfrequency.list.value.size())
		{
			if (!format.empty())
				format += "_";
			format += "GPUMEMF";
		}
		if (parameters.GPUcapPower.list.value.size())
		{
			if (!format.empty())
				format += "_";
			format += "GPUPWR";
		}

		return format;
	}

};


int loadConfigFile(json_object_element_s* data, CFGbase * cfg)
{
	while(data != NULL)
	{
//		std::cout << " ** "<< std::string(data->name->string) << std::endl;
//		std::cout << "CFG size: " << cfg->config.size() << std::endl;
		
		auto it = cfg->config.find(std::string(data->name->string));
		if (it == cfg->config.end())
		{
			std::cerr << "Unsupported parameter '" << data->name->string << "' in input JSON file\n";
			return 1;
		}
		else
		{
			if (it->second->lastLevel())
			{
				if (data->value->type == json_type_array)
				{
					struct json_array_element_s* lst = ((struct json_array_s*)data->value->payload)->start;
					do
					{
						it->second->setValue(std::string(((struct json_string_s*)lst->value->payload)->string));
						lst = lst->next;
					}while(lst != NULL);
				}
				else
					it->second->setValue(std::string(((struct json_string_s*)data->value->payload)->string));
					//TODO if return 1
			}
			else
			{
				//func(((struct json_object_s*)data->value->payload)->start, it->second);
				if (loadConfigFile(((struct json_object_s*)data->value->payload)->start, it->second) == 1)
					return 1;
			}
		}
		data = data->next;
	}
	return 0;
}

//} //namespace

#endif // #define SRC_TOOLS_WRAPPER_CONFIG_H_
