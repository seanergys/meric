
#ifndef SRC_TOOLS_WRAPPER_ALGORITHMS_RANDOMTEST_HPP_
#define SRC_TOOLS_WRAPPER_ALGORITHMS_RANDOMTEST_HPP_

#include "../parameter/statespace.h"

class randomTestSearch : public configurationStateSpace {

public:
	/**
	 * The only public method - generates new configurations in which the application should be executed
	 * if the state-space-search is over, the vector will remain empty
	 *
	 * The randomtest method randomly generates 3 configurations of each region of the application
	 * the second call of the method results in analysis of the previously executed configrations only // TODO 3. call evaluation
	 *
	 * space [in-out]: vector to be filled in with configurations in which the application will be executed
	 * finished [in]: vector of already finished configurations, to evaluated them
	 * returns amount of newly generated configurations (size of the std::vector space)
	 **/
	int generateConfigurations(std::vector<appConfiguration> & space, std::vector<appConfiguration> & finished)
	{
		if (finished.size())
		{
			DataAnalysis::analyse(finished);
			return 0;
		}
		
		if (space.size())
		{
			srand(time(NULL));
		}
		
std::cout << "CSS params size = " << configurationStateSpace::parameters.size() << "\n";
		
		for (int i=0; i<MAGIC_CONST_NUM_OF_CONF_TO_GENERATE; i++)
		{
			appConfiguration ac;
			for (auto region : DataAnalysis::regions)
			{
				regionConfiguration rc(region);
				//TODO generate conf
					//for(int paramID=0; paramID < parameters.size(); paramID++)
					for (auto param : configurationStateSpace::parameters)
					{
						int valueID = rand() % param->getSize();
						parameterValueNew pv(param->getParameterID(), valueID);
						rc.addParameterValue(pv, std::bind(&parameterBase::setParameterValue, param, valueID), std::bind(&parameterBase::getParameterValue, param, valueID));
					}
				
				//TODO test if the conf has not been generated previously
				ac.push_back(rc);
			}
			space.push_back(ac);
		}
		
		return space.size();
	}

private:
	const int MAGIC_CONST_NUM_OF_CONF_TO_GENERATE = 3;
};


#endif // SRC_TOOLS_WRAPPER_ALGORITHMS_RANDOMTEST_HPP_

