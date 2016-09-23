
#include "utils.h"

namespace meric {

template <>
inline std::string mutils::getenv(const std::string &param, std::string defaultValue)
{
	std::string value;
	char *var = std::getenv(param.c_str());

	if (var != NULL) {
		std::stringstream ss(var);
		return ss.str();
	} else {
		return defaultValue;
	}
}

template <>
inline const char* mutils::getenv(const std::string &param, const char* defaultValue)
{
	char *var = std::getenv(param.c_str());

	if (var != NULL) {
        std::stringstream ss(var);
		return ss.str().c_str();
	} else {
		return defaultValue;
	}
}

template <>
inline bool mutils::getenv(const std::string &param, bool defaultValue)
{
	bool value;
	char *var = std::getenv(param.c_str());
	if (var != NULL) {
		std::string str(var);
		if (str.size() == 0) {
			return true;
		}
		std::stringstream ss(var);
		ss >> value;
		return value;
	} else {
		return defaultValue;
	}
}

template <typename Ttype>
Ttype mutils::getenv(const std::string &param, Ttype defaultValue)
{
	Ttype value;
	char *var = std::getenv(param.c_str());

	if (var != NULL) {
		std::stringstream ss(var);
		ss >> value;
		return value;
	} else {
		return defaultValue;
	}
}

}
