
#ifndef SRC_TOOLS_WRAPPER_PARAMETER_COREFREQUENCY_HPP_
#define SRC_TOOLS_WRAPPER_PARAMETER_COREFREQUENCY_HPP_

#include "parameter.h"
#include <string>

/*
	template <typename Ttype>
	class coreFrequency : public parameterBase<Ttype> {
		...
	}

	coreFrequency<double> CFparam;
*/

//template <typename Ttype> //<class T = Ttype>
class coreFrequency : public parameterBase { //<Ttype> {

//singleton initialization
public:
	typedef long ParType;
	coreFrequency(coreFrequency const&)  = delete;
	void operator=(coreFrequency const&) = delete;


	const parameterType type = DYNAMIC;
	
//*
	coreFrequency* getInstance()
	{
		MERIC_CONFIG_STRING = "FREQUENCY";
		//static coreFrequency instance;
std::cout << "*0\n";
		return (!instance) ? instance = new coreFrequency : instance;
//		instance = new coreFrequency();
//		return (instance);
		//return (&instance);
	}
	
	coreFrequency* getInstance(std::vector<long> & values)
	{
		//coreFrequency * c = this->getInstance();
		if (!instance) 
			instance = new coreFrequency;
std::cout << "*1\n";


		meric::systemLimits limit = meric::Environment::getSystemSettings();
			std::vector<long> CFvalueList = values; //rm
			instance->defaultValue = values.back(); //rm
		/* TODO nyni vraci prazdy vector  - zprovoznit pri kompilovani spolu s MERIC
		std::vector<long> CFvalueList;
		meric::Environment::getAvailableCF(CFvalueList);

		
		std::cout << "CF from MERIC: " << limit.defaultCoreFreq << " ";
		for (auto x : CFvalueList)
			std::cout << x << " ";
		std::cout << std::endl;

		//only valid values are inserted		
		for (int i=0; i<CFvalueList.size(); i++)
		{
			if(std::find(values.begin(), values.end(), CFvalueList[i]) == values.end())
			{
				CFvalueList.erase(CFvalueList.begin()+i);
				i--;
			}
		}
		instance->defaultValue = limit.defaultCoreFreq;
		*/
		instance->validValues = CFvalueList;
				
		return instance;
	}
//*/

	static coreFrequency* getInterfaceInstance()
	{
		MERIC_CONFIG_STRING = "FREQUENCY";
//std::cout << "*0\n";
		return (!instance) ? instance = new coreFrequency : instance;
//		instance = new coreFrequency();
//		return (instance);
		//return (&instance);
	}
	
	void printList()	//TMP
	{
		coreFrequency * c = getInstance();
		if (c->validValues.size())
		{
			std::cout << "list: ";
			for (auto x : c->validValues)
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
		coreFrequency * c = getInstance();
//std::cout << "CORE FREQ set " << c->validValues[id] << std::endl;
		setenv(("MERIC_"+MERIC_CONFIG_STRING).c_str(), std::to_string(c->validValues[id]).c_str(), 1);
		return "\""+MERIC_CONFIG_STRING+"\" : "+std::to_string(c->validValues[id]);
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
	coreFrequency() {};
	~coreFrequency() {};
	
	static std::vector <long> validValues;
	static std::string MERIC_CONFIG_STRING;


	static coreFrequency *instance;
	static int parameterID;
	
	//	static long minValue;
//	static long maxValue;
	static long defaultValue; // ?
//	static std::vector <long> validValues;
};


int coreFrequency::parameterID = -1;
coreFrequency* coreFrequency::instance = nullptr;
std::vector <long> coreFrequency::validValues;
long coreFrequency::defaultValue;
std::string coreFrequency::MERIC_CONFIG_STRING;

#endif // SRC_TOOLS_WRAPPER_PARAMETER_COREFREQUENCY_HPP_
