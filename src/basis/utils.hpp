
#include "utils.h"

namespace meric {

template <>
inline bool mutils::getenv(const std::string &param, bool defaultValue)
{
	bool value;
	char *var = std::getenv(param.c_str());
	if (var != NULL)
	{
		std::string str(var);
		if (str.size() == 0)
		{
			return true;
		}
		std::stringstream ss(var);
		ss >> value;
		return value;
	}
	else
	{
		return defaultValue;
	}
}

template <typename Ttype>
Ttype mutils::getenv(const std::string &param, Ttype defaultValue)
{
	Ttype value;
	char *var = std::getenv(param.c_str());

	if (var != NULL)
	{
		std::stringstream ss(var);
		if (ss.str().size() == 0)
			return defaultValue;
		ss >> value;
		return value;
	}
	else
	{
		return defaultValue;
	}
}

}
