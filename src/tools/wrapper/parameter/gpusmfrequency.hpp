
#ifndef SRC_TOOLS_WRAPPER_PARAMETER_GPUSMFREQUENCY_HPP_
#define SRC_TOOLS_WRAPPER_PARAMETER_GPUSMFREQUENCY_HPP_

#include "parameter.h"
#include <string>


class gpuSMfreqScaling : public parameterBase {

//singleton initialization
public:
	typedef long ParType;
	gpuSMfreqScaling(gpuSMfreqScaling const&)  = delete;
	void operator=(gpuSMfreqScaling const&) = delete;


	const parameterType type = DYNAMIC;
	

	gpuSMfreqScaling* getInstance()
	{
		MERIC_CONFIG_STRING = "GPU_FREQUENCY";
		return (!instance) ? instance = new gpuSMfreqScaling : instance;
	}
	
	gpuSMfreqScaling* getInstance(std::vector<long> & values)
	{
		if (!instance) 
			instance = new gpuSMfreqScaling;

		meric::systemLimits limit = meric::Environment::getSystemSettings();
			std::vector<long> GPUSMvalueList = values; //rm
			instance->defaultValue = values.back(); //rm
		//TODO get the maximum/minimum available value
		/* TODO nyni vraci prazdy vector  - zprovoznit pri kompilovani spolu s MERIC
		std::vector<long> THRDSvalueList;
			meric::Environment::initHWSizes(int &sn, int &cs, int &tc, std::vector<int> &online)

		...

		//only valid values are inserted
		...
		*/
		instance->validValues = GPUSMvalueList;
				
		return instance;
	}

/*
	static coreFrequency* getInterfaceInstance()
	{
		MERIC_CONFIG_STRING = "FREQUENCY";
std::cout << "*0\n";
		return (!instance) ? instance = new coreFrequency : instance;
//		instance = new coreFrequency();
//		return (instance);
		//return (&instance);
	}
*/	
	void printList()	//TMP
	{
		gpuSMfreqScaling * c = getInstance();
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

	long getRandomValue() //? TODO - do I need this method, or it should provide random value ID
	{
		return validValues.size() ? validValues[rand() % validValues.size()] : -1;
	}

	void unsetParameter()
	{
		setenv(("MERIC_"+MERIC_CONFIG_STRING).c_str(), "0", 1);
	}
	
	std::string setParameterValue (int id)
	{
		gpuSMfreqScaling * c = getInstance();
//std::cout << MERIC_CONFIG_STRING <<" set " << c->validValues[id] << std::endl;
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
	gpuSMfreqScaling() {};
	~gpuSMfreqScaling() {};
	
	static std::vector <long> validValues;
	static std::string MERIC_CONFIG_STRING;


	static gpuSMfreqScaling *instance;
	static int parameterID;
	
	//	static long minValue;
//	static long maxValue;
	static long defaultValue; // ?
//	static std::vector <long> validValues;
};


int gpuSMfreqScaling::parameterID = -1;
gpuSMfreqScaling* gpuSMfreqScaling::instance = nullptr;
std::vector <long> gpuSMfreqScaling::validValues;
long gpuSMfreqScaling::defaultValue;
std::string gpuSMfreqScaling::MERIC_CONFIG_STRING;

#endif // SRC_TOOLS_WRAPPER_PARAMETER_GPUSMFREQUENCY_HPP_
