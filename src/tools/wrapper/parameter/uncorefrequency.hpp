
#ifndef SRC_TOOLS_WRAPPER_PARAMETER_UNCOREFREQUENCY_HPP_
#define SRC_TOOLS_WRAPPER_PARAMETER_UNCOREFREQUENCY_HPP_

#include "parameter.h"
#include <string>

class uncoreFrequency : public parameterBase {

//singleton initialization
public:
	typedef long ParType;
	uncoreFrequency(uncoreFrequency const&)  = delete;
	void operator=(uncoreFrequency const&) = delete;


	const parameterType type = DYNAMIC;
	
	uncoreFrequency* getInstance()
	{
		MERIC_CONFIG_STRING = "UNCORE_FREQUENCY";
		return (!instance) ? instance = new uncoreFrequency : instance;
	}
	
	uncoreFrequency* getInstance(std::vector<long> & values)
	{
		if (!instance)
			instance = new uncoreFrequency;
		
		meric::systemLimits limit = meric::Environment::getSystemSettings();
			std::vector<long> UCFvalueList = values; //rm
			instance->defaultValue = values.back(); //rm
		/*  TODO nyni vraci prazdy vector  - zprovoznit pri kompilovani spolu s MERIC
		std::vector<long> UCFvalueList;
		meric::Environment::getAvailableUCF(UCFvalueList);

		//only valid values are inserted		
		for (int i=0; i<UCFvalueList.size(); i++)
		{
			if(std::find(values.begin(), values.end(), UCFvalueList[i]) == values.end())
			{
				UCFvalueList.erase(UCFvalueList.begin()+i);
				i--;
			}
		}
		instance->defaultValue = limit.defaultCoreFreq;
		*/
		instance->validValues = UCFvalueList;
		return instance;
	}

	void printList()	//TMP
	{
		uncoreFrequency * u = getInstance();
		if (u->validValues.size())
		{
			std::cout << "list: ";
			for (auto x : u->validValues)
				std::cout << x << " ";
			std::cout << std::endl;
		}
		else
			std::cout << "empty list\n";
	}

	//returns ID of the value, otherwise -1
	//if optional parameters add is true, the value is added if missing
	int isValidValue(long value, bool add = false)
	{
		int i = 0;
		while (i < validValues.size())
		{
			if (validValues[i] == value)
				return i;
			i++;
		}
		if (add == true)
		{
			validValues.push_back(value);
			return i;
		}
		return -1;
	}

	long getValue(size_t i)
	{
		return i < validValues.size() ?  validValues[i] : -1;
	}

	long getRandomValue()
	{
		return validValues.size() ? validValues[rand() % validValues.size()] : -1;
	}
	
	void unsetParameter()
	{
		setenv(("MERIC_"+MERIC_CONFIG_STRING).c_str(), "0", 1);
	}
	
	std::string setParameterValue (int id)
	{
		uncoreFrequency * u = getInstance();
//std::cout << "UNCORE FREQ set " << u->validValues[id] << std::endl;
		setenv(("MERIC_"+MERIC_CONFIG_STRING).c_str(), std::to_string(u->validValues[id]).c_str(), 1);
		return "\""+MERIC_CONFIG_STRING+"\" : "+std::to_string(u->validValues[id]);
	}
	
	std::string getParameterValue(int id)
	{
		return std::to_string(validValues[id]);
	}
	
	int getSize() override
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

private:
	uncoreFrequency() {};
	~uncoreFrequency() {};
	
	static std::vector <long> validValues;
	static std::string MERIC_CONFIG_STRING;
	

	static int parameterID;
	static uncoreFrequency *instance;
	static long defaultValue; // ?

};


int uncoreFrequency::parameterID = -1;
uncoreFrequency* uncoreFrequency::instance = nullptr;
std::vector <long> uncoreFrequency::validValues;
long uncoreFrequency::defaultValue;
std::string uncoreFrequency::MERIC_CONFIG_STRING;

#endif // SRC_TOOLS_WRAPPER_PARAMETER_UNCOREFREQUENCY_HPP_
