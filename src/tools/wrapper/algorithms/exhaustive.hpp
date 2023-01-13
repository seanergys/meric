
#ifndef SRC_TOOLS_WRAPPER_ALGORITHMS_EXHAUSTIVE_HPP_
#define SRC_TOOLS_WRAPPER_ALGORITHMS_EXHAUSTIVE_HPP_

#include "../parameter/statespace.h"

class exhaustiveSearch : public configurationStateSpace {

public:
	/**
	 * The only public method - generates new configurations in which the application should be executed
	 * if the state-space-search is over, the vector will remain empty
	 *
	 * The exhaustive search generates static configurations, all the configurations are generated at once
	 * the second call of the method results in analysis of the previously executed configrations only
	 *
	 * space [in-out]: vector to be filled in with configurations in which the application will be executed
	 * finished [in]: vector of already finished configurations, to evaluated them
	 * returns amount of newly generated configurations (size of the std::vector space)
	 **/
	int generateConfigurations(std::vector<appConfiguration> & space, std::vector<appConfiguration> & finished)
	{
std::cout << "IN: S("<<space.size()<<") F("<<finished.size()<<")\n";
	
		if (finished.size())
		{
			DataAnalysis::analyse(finished);
std::cout << "OUT: S("<<space.size()<<") F("<<finished.size()<<")\n";
			return 0;
		}

		regionConfiguration conf(DataAnalysis::regions[0]);
		std::cout << typeid(*parameters[0]).name() << " size: "<< parameters[0]->getSize() <<'\n';
		std::cout << parameters[0] << std::endl;
////////////////
const auto p = parameters[0];
//parameterValue<__typeof__(*p)> pv(0);
//conf.addParameterValue(std::bind(&parameterValue<__typeof__(*parameters[0])>::setParameter, &pv));
//space.push_back(conf);
////////////////
pokus<__typeof__(*parameters[0])> p1;
pokus<__typeof__(parameters[0])> p2;
//pokus<__typeof__(**parameters[0])> p3;
pokus<__typeof__(coreFrequency)> p4;
pokus<__typeof__(parameters)> p5;
////////////////

		
//		xxx(0, conf, space);

/*
		for (auto param : parameters)
		{
			std::cout << param << std::endl;
			std::cout << typeid(*param).name() << " size: "<< param->getSize() <<'\n';
			for (int valID = 0; valID < param->getSize(); valID++)
			{
				parameterValue<__typeof__(*param)> pv(valID);
				regionConfiguration conf; //empty region name
				conf.addParameterValue(std::bind(&parameterValue<__typeof__(*param)>::setParameter, &pv));
				space.push_back(conf);
			}
		}
//*/

		generateAllConfigurationsRecursive(0, conf, space); //TODO paramID
		std::cout << "we have generated "<<space.size() << " configurations\n";

std::cout << "OUT: S("<<space.size()<<") F("<<finished.size()<<")\n";
		return space.size();
	}

private:
	/**
	 * the exhaustive search generates a full list of possible configurations of each tuned parameter
	 * the implementation uses recursion
	 * the configurations are static, so only for the static region
	 **/
	void generateAllConfigurationsRecursive(int paramID, regionConfiguration conf, std::vector<appConfiguration> &space)
	{
		auto param = parameters[paramID];
		std::cout << param << " + " << typeid(*parameters[0]).name() << " SS size " << parameters.size() << std::endl;

		if(paramID+1 < parameters.size())
		{
			for (int i = 0; i < param->getSize(); i++)
			{
				regionConfiguration c(conf);
				parameterValueNew pv(paramID, i);
				//c.addParameterValue(pv, std::bind(&parameterBase::setParameterValue, param, i)); //__typeof__(*param)
				c.addParameterValue(pv, std::bind(&parameterBase::setParameterValue, param, i), std::bind(&parameterBase::getParameterValue, param, i));
				generateAllConfigurationsRecursive(paramID+1, c, space);
			}
		}
		else
		{
			for (int i = 0; i < param->getSize(); i++)
			{
				regionConfiguration c(conf);
				parameterValueNew pv(paramID, i);
				//c.addParameterValue(pv, std::bind(&parameterBase::setParameterValue, param, i));
				c.addParameterValue(pv, std::bind(&parameterBase::setParameterValue, param, i), std::bind(&parameterBase::getParameterValue, param, i));
				appConfiguration appC = {c};
				space.push_back(appC);
			}
		}
	}
};


#endif // SRC_TOOLS_WRAPPER_ALGORITHMS_EXHAUSTIVE_HPP_

