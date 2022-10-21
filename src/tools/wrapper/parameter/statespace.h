
#ifndef SRC_TOOLS_WRAPPER_PARAMETER_STATESPACE_H_
#define SRC_TOOLS_WRAPPER_PARAMETER_STATESPACE_H_

#include "../evaluate/dataanalysis.h"

//#include "parameter.h"
#include "corefrequency.hpp"
#include "uncorefrequency.hpp"
#include "threadsscaling.hpp"
// #include "cappowercpu.hpp"	//TODO
// #include "captimecpu.hpp"	//TODO
#include "gpusmfrequency.hpp"
#include "gpumemfrequency.hpp"
// #include "cappowergpu.hpp"	//TODO
#include "dummyparameter.hpp"	//in use for post-analsis




////////////////////////////////////////////////////////////////////////////////
enum ALGORITHM {EXHAUSTIVE=0, EVO=1, PSO=2, NONE}; //TODO RANDOM ? pouziva se toto enum nekde ?

class configurationStateSpace {

public:
	static std::vector<parameterBase *> parameters;
	
	virtual int generateConfigurations(std::vector<appConfiguration> & space, std::vector<appConfiguration> & finished) = 0;
/*
	//must be called before the analysis runs starts - TODO pridat kontrolu
	static void StateSpaceClose()
	{
		meric::Environment::close();
	}
*/

	//static 
	int constructStateSpace(cfg & config)
	{
		//analysis of already measured data - load the config
		if (config.algorithm.name.value == config.algorithm.name.supportedValues.back())
			return 0;

		meric::RegionParameters defaultParameter;
		meric::Environment::init(meric::SYS_OTHER, defaultParameter, 0);
		
		if (config.parameters.frequency.list.value.size())
		{
			coreFrequency * CFparam;
			CFparam = CFparam->getInstance(config.parameters.frequency.list.value);
			parameters.push_back(CFparam);
			CFparam->setParameterID(++SSsize);
			CFparam->printList();
		}
		
		if (config.parameters.uncorefrequency.list.value.size())
		{
			uncoreFrequency * UCFparam;
			UCFparam = UCFparam->getInstance(config.parameters.uncorefrequency.list.value);
			parameters.push_back(UCFparam);
			UCFparam->setParameterID(++SSsize);
			UCFparam->printList();
		}
		
		if (config.parameters.threads.list.value.size()) 
		{
			threadsScaling * THRDSparam;
			THRDSparam = THRDSparam->getInstance(config.parameters.threads.list.value);
			parameters.push_back(THRDSparam);
			THRDSparam->setParameterID(++SSsize);
			THRDSparam->printList();
		}
/*		//TODO
		if (config.parameters.capPower.list.value.size())
		{
			<> * CPUCAPparam;
			CPUCAPparam = CPUCAPparam->getInstance(config.parameters.capPower.list.value);
			parameters.push_back(CPUCAPparam);
			CPUCAPparam->setParameterID(++SSsize);
			CPUCAPparam->printList();
		}
		if (config.parameters.capTime.list.value.size())
		{
			<> * CPUCAPTIMEparam;
			CPUCAPTIMEparam = CPUCAPTIMEparam->getInstance(config.parameters.capTime.list.value);
			parameters.push_back(CPUCAPTIMEparam);
			CPUCAPTIMEparam->setParameterID(++SSsize);
			CPUCAPTIMEparam->printList();
		}
*/		if (config.parameters.GPUSMfrequency.list.value.size())
		{
			gpuSMfreqScaling * GPUSMparam;
			GPUSMparam = GPUSMparam->getInstance(config.parameters.GPUSMfrequency.list.value);
			parameters.push_back(GPUSMparam);
			GPUSMparam->setParameterID(++SSsize);
			GPUSMparam->printList();
		}
		if (config.parameters.GPUMEMfrequency.list.value.size())
		{
			gpuMEMfreqScaling * GPUMEMparam;
			GPUMEMparam = GPUMEMparam->getInstance(config.parameters.GPUMEMfrequency.list.value);
			parameters.push_back(GPUMEMparam);
			GPUMEMparam->setParameterID(++SSsize);
			GPUMEMparam->printList();
		}
/*		if (config.parameters.GPUcapPower.list.value.size())
		{
			<> * GPUCAPparam;
			GPUCAPparam = GPUCAPparam->getInstance(config.parameters.GPUcapPower.list.value);
			parameters.push_back(GPUCAPparam);
			GPUCAPparam->setParameterID(++SSsize);
			GPUCAPparam->printList();
		}
*/
		meric::Environment::close();

		std::cout << "SS size = " << SSsize << "\n";
		return SSsize;
	}
	
/*	static int addParameterToTune(parameterBase * param)
	{
		parameters.push_back(param);
		return ++SSsize; //parameter ID
	}
*/	
/*	static int getParameterID(parameterBase * param)
	{
		auto it = std::find(parameters.begin(), parameters.end(), param);
		if (it != parameters.end())
			return distance(parameters.begin(), it);
		else
			return -1;
	}
*/	
	//static 
	size_t getDimensionsSize() { return SSsize+1; };
	//static size_t getDimensionSize(size_t i) { return parameters[i].validValues.size(); };
	
//	regionConfiguration getRandomConfig()
//	{
//		//for (parameterBase parameter : parameters)
//		//	parameter.getRandomValue();
//	}

	void cleanEnvironment()
	{
		unsetenv("MERIC_REGION_OPTIONS");
		for (auto param : parameters)
			param->unsetParameter();
//		if (MERIC_OUTPUT_FILENAME != "")
//			remove(MERIC_OUTPUT_FILENAME.c_str());
	}

	configurationStateSpace()
	{
		SSsize = -1;
	}
private:
	//static 
	int SSsize;

	
	/*
	void addParamterValue(regionConfiguration & c, coreFrequency<long> &parameter, long &val)
	{
		parameterValue<coreFrequency<long>> pv(val);
		c.addParameterValue(std::bind(&parameterValue<coreFrequency,long>::setParameter, &pv));
	}
	*/
/*	
	static void xxx(int paramID, regionConfiguration conf, std::vector<regionConfiguration> &space)
	{
		auto param = parameters[paramID];
		std::cout << param << " - " << typeid(*parameters[0]).name() << std::endl;

	//	if (valID+1 < param->getSize())
		
		if(paramID+1 < parameters.size())
		{
			for (int i = 0; i < param->getSize(); i++)
			{
				regionConfiguration c(conf);
				parameterValue<__typeof__(*param)> pv(i);
				c.addParameterValue(std::bind(&parameterValue<__typeof__(*param)>::setParameter, &pv));
				xxx(paramID++, c, space);
			}
		}
		else
		{
			for (int i = 0; i < param->getSize(); i++)
			{
				regionConfiguration c(conf);
				parameterValue<__typeof__(*param)> pv(i);
				c.addParameterValue(std::bind(&parameterValue<__typeof__(*param)>::setParameter, &pv));
				space.push_back(c);
			}
		}
	}
*/



};


//int configurationStateSpace::SSsize;
std::vector<parameterBase *> configurationStateSpace::parameters;
//std::vector<int *> configurationStateSpace::tmp;

#endif //SRC_TOOLS_WRAPPER_PARAMETER_STATESPACE_H_
