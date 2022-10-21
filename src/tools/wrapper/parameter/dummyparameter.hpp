#ifndef SRC_TOOLS_WRAPPER_PARAMETER_DUMMYPARAMETER_HPP_
#define SRC_TOOLS_WRAPPER_PARAMETER_DUMMYPARAMETER_HPP_

#include "parameter.h"
#include <string>

class dummyparameter : public parameterBase {	//aktualne se nevyuziva dedicnosti

//not a singleton class
public:
	typedef long ParType;
	const parameterType type = DYNAMIC;
	
	dummyparameter(std::string name = "") : MERIC_CONFIG_STRING(name)
	{
		parameterID = -1;
	};
	
	dummyparameter(std::string name, std::vector <std::string> v) : validValues(v), MERIC_CONFIG_STRING(name)
	{
		parameterID = -1;
	};

	void printList()	//TMP
	{
		std::cout << MERIC_CONFIG_STRING << " list: ";
		for (auto x : validValues)
			std::cout << x << " ";
		std::cout << std::endl;
	}
	
	int isValidValue(std::string value, bool add = false) // TODO pridat do ostatnich
	{
		int pos = -1;
		std::vector<std::string>::iterator it = std::find(validValues.begin(), validValues.end(), value);
		if (it == validValues.end())
		{
			if (add)
			{
				validValues.push_back(value);
				pos = validValues.size()-1;
			}
			else
				pos = -1;
		}
		else
			pos = it - validValues.begin();
		return pos;
	}

	std::string getValue(size_t i)
	{
		return i < validValues.size() ?  validValues[i] : "";
	}
	
	std::string getParameterValue(int id)
	{
//		std::cout << "GET " << validValues[id] << std::endl;
		return validValues[id];
	}
	
	std::string setParameterValue (int id)
	{
//		std::cout << "SET " << validValues[id] << std::endl;
		if (MERIC_CONFIG_STRING == "CF")
			return "\t\"FREQUENCY\" : "+validValues[id];
		else if (MERIC_CONFIG_STRING == "UnCF")
			return "\t\"UNCORE_FREQUENCY\" : "+validValues[id];
		else if (MERIC_CONFIG_STRING == "thrds")
			return "\t\"NUM_THREADS\" : "+validValues[id];
		else if (MERIC_CONFIG_STRING == "CPUPWR")
			return "\t\"PWRCAP_POWER\" : "+validValues[id];
		else if (MERIC_CONFIG_STRING == "CPUPWRtw")
			return "\t\"PWRCAP_TIME\" : "+validValues[id];
		else if (MERIC_CONFIG_STRING == "GPUF")
			return "\t\"GPU_FREQUENCY\" : "+validValues[id];
		else if (MERIC_CONFIG_STRING == "GPUMEMF")
			return "\t\"GPU_MEM_FREQUENCY\" : "+validValues[id];
		else if (MERIC_CONFIG_STRING == "GPUPWR")
			return "\t\"PWRCAP_GPU\" : "+validValues[id];
		else	//node, TODO APP
			return ""; 
	}
	
	int getSize()
	{
		return validValues.size();
	}
	
	void setParameterID(int id)
	{
		parameterID = id;
	}
	
	int getParameterID()
	{
		return parameterID;
	}
	
	void unsetParameter() {}
	parameterBase* getInstance() {return nullptr;}

private:
	std::vector <std::string> validValues;
	std::string MERIC_CONFIG_STRING;
	int parameterID;
};

#endif // SRC_TOOLS_WRAPPER_PARAMETER_DUMMYPARAMETER_HPP_
