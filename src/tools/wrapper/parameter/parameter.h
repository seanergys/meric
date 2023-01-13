
#ifndef SRC_TOOLS_WRAPPER_PARAMETER_PARAMETER_H_
#define SRC_TOOLS_WRAPPER_PARAMETER_PARAMETER_H_

#include <vector>
#include <functional>
#include <memory>

#include "../../../basis/utils.h"
#include "../../../wrapper/environmentwrapper.h"

#include "../config.h"


////////////////////////////////////////////////////////////////////////////////
enum parameterType {STATIC, DYNAMIC, CFGFILE };

class parameterBase {

public:
	virtual std::string setParameterValue(int id) = 0;
	virtual std::string getParameterValue(int id) = 0;
	virtual void unsetParameter() = 0;
	virtual int getSize() = 0;
	virtual parameterBase* getInstance() = 0;
	virtual void printList() = 0;
	virtual void setParameterID(int id) = 0;
	virtual int getParameterID() = 0;
	static parameterBase* inst;	// ?
};

/*
//special class that suits for instancing singletons that heritage from a base class, that cannot be templated to be vector template data type
template<class T>
class parameterInterface{

public:
	//typedef typename T::ParType Ttype;
	static T getInstance()
	{
		T x;
		return x->getInterfaceInstance();
	}
	
};
*/


////////////////////////////////////////////////////////////////////////////////

template <class Tparam>
class pokus {

public:
	pokus()
	{
		Tparam * p;
		std::cout << "POKUS: "<< typeid(p).name() << '\n';
	}
};

/*
template <class Tparam>
class parameterValue {

public:
	//typedef typename Tparam::ParType Ttype;

	parameterValue(int valID) : valueID(valID) 
	{

	
		Tparam * p;
		std::cout << "ParVal 2: "<< typeid(p).name() << '\n';
		
		//parameterInterface<Tparam *> i;
//		parameterInterface<__typeof__(p)> i;
		//p = i.getInstance();
	

		parameter = p->getInstance();
		parameter->printList();
		if (valID >= parameter->getSize())
		{
			std::cerr << "ERROR parameter value overflow\n"; //throw
			valueID = -1;
		}
	};
	
	void setParameter()	//virtual
	{
		parameter->setParameterValue(valueID);
	}

//	listedParameterBase getParameterType()	//virtual
//	{
//		return parameter;
//	}
	
	
	inline bool operator==(const int & valID) const
	{
		return this->valueID == valID;
	}	
	
	inline bool operator==(const parameterValue<Tparam>& paramVal) const
	{
		return this->parameter == paramVal.parameter && this->valueID == paramVal.valueID;
	}
	
private:
	//const Ttype value;
	int valueID;
	Tparam * parameter;
};
*/
/*
template<template <class Ttype> class Tparam, typename T>
class parameterValue
{
public:
	parameterValue(T v) : value(v) 
	{
		parameter = parameter->getInstance();
		parameter->printList(); //RM
	};
	
	void setParameter()	//virtual
	{
		parameter->setParameterValue(value);
	}

	std::string getValue() const	//virtual
	{
		return std::to_string(value);
	}
	
	listedParameterBase getParameterType()	//virtual
	{
		return parameter;
	}
	
	
	inline bool operator==(const T& val) const
	{
		return val==value;
	}

	inline bool operator==(const parameterValue<Tparam, T>& paramVal) const
	{
		return this->getValue() == paramVal.getValue();
	}	
	
private:
	Tparam<T> * parameter;
	const T value;
};
*/


class parameterValueNew {	//TODO rename

public:

	parameterValueNew(int parID, int valID) : parameterID(parID), valueID(valID) {}
	
	inline bool operator==(const int & valID) const
	{
		return this->valueID == valID;
	}	
	
	inline bool operator==(const parameterValueNew& paramVal) const
	{
		return this->parameterID == paramVal.parameterID && this->valueID == paramVal.valueID;
	}
	
private:
	int valueID;
	int parameterID;
};

////////////////////////////////////////////////////////////////////////////////
class ResourcesConsumption {


public:
	std::vector<std::string> callpath;
	std::vector<double> runtime; //each element is a single call of the region
	std::vector<double> energy;
	
	double getAvgEnergy()
	{
		double sum = .0;
		if (energy.size())
		{
			for (double e : energy)
				sum +=e;
			sum /= energy.size();
		}
		return sum;
	}
	
	double getAvgTime()
	{
		double sum = .0;
		if (runtime.size())
		{
			for (double t : runtime)
				sum +=t;
			sum /= runtime.size();
		}
		return sum;
	}
	
	//inline bool operator += (const ResourcesConsumption& cons) const
	//ResourcesConsumption& ResourcesConsumption::operator+=(const ResourcesConsumption& cons)
	inline ResourcesConsumption& operator+=(const ResourcesConsumption& cons)
	{
		this->callpath.insert(this->callpath.end(), cons.callpath.begin(), cons.callpath.end());
		this->runtime.insert(this->runtime.end(), cons.runtime.begin(), cons.runtime.end());
		this->energy.insert(this->energy.end(), cons.energy.begin(), cons.energy.end());
		
		return *this;
	}
};

////////////////////////////////////////////////////////////////////////////////
class regionConfiguration {

public:
	std::vector<parameterValueNew> values; //parameters configuration
	std::string regionName;
	ResourcesConsumption consumption;
	
	regionConfiguration(std::string reg="") : regionName(reg)
	{
	}
	
	regionConfiguration(const regionConfiguration & c) : regionName(c.regionName), consumption(c.consumption), setParameters(c.setParameters), getParameters(c.getParameters), values(c.values)
	{
	}
	
	void print()
	{
		std::cout << regionName << ": ";
		for(auto& get : getParameters)
			std::cout << get() << " ";
		std::cout << "T: " << consumption.getAvgTime() << " E: " << consumption.getAvgEnergy() << std::endl;
	}
	
	void addParameterValue(parameterValueNew pv)
	{
		values.push_back(pv);
	}
	
	void addParameterValue(std::function<std::string()> set)
	{
		setParameters.push_back(set);
	}
	
	void addParameterValue(parameterValueNew pv, std::function<std::string()> set, std::function<std::string()> get)
	{
		values.push_back(pv);
		setParameters.push_back(set);
		getParameters.push_back(get);
//		std::cout << "NEW cfg - " << get() << " SIZE: " << setParameters.size() <<"x"<<getParameters.size() << std::endl;
	}
			
	std::string setConfiguration()
	{
//std::cerr << "SET REG CONF [" << regionName << "]\n";
	
		std::string json = "";
		for(auto& set : setParameters)
		{
			std::string token = set();
			if (token != "")
			{
				json += "\t\t" + token + ",\n";
			}
		}
		if (json != "")
		{
			json = json.substr(0,json.length()-2);
			if (regionName != "")
			{
				json = "\t\""+regionName+"\" : {\n"+json+"\n\t}\n";
			}			
		}
		return json;
	}
	
	std::string constructConfigFileName(std::string hostname = "")
	{
		std::string cfgStr = "";
		for(auto& get : getParameters)
			cfgStr += get() + "_";
		if (cfgStr.back() == '_')
			cfgStr.pop_back();

		if (hostname != "")
			cfgStr = hostname +"_"+ cfgStr;
		
		return regionName+"/"+cfgStr+".csv";
	}

private:
	std::vector<std::function<std::string()>> setParameters;
	std::vector<std::function<std::string()>> getParameters;
	
};

////////////////////////////////////////////////////////////////////////////////
typedef std::vector <regionConfiguration> appConfiguration;


void storeMERICconfig(appConfiguration & configuration, std::string outputFile)
{
	std::string mericConfig = "";
	for(auto region : configuration)
	{
		std::string token = region.setConfiguration();
		if (token != "")
			mericConfig += token+",\n";
	}
	
	if (mericConfig != "")
	{
		std::ofstream configFile(outputFile);
		if(configFile.is_open())
		{
			configFile << "{\n" << mericConfig.substr(0, mericConfig.length()-2) << "\n}\n";
			configFile.close();
//std::cout << "{\n" << mericConfig.substr(0, mericConfig.length()-2) << "\n}\n";
		}
		else
		{
			std::cerr << "MERICwrapper ERROR: failed to store application configuration to " << outputFile << std::endl;
			exit(1);
		}
		setenv("MERIC_REGION_OPTIONS", outputFile.c_str(), 1);
	}
	else
	{
		unsetenv("MERIC_REGION_OPTIONS");
	}
}


#endif // SRC_TOOLS_WRAPPER_PARAMETER_PARAMETER_H_
