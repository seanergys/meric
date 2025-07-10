#include <sstream>
#include <iomanip>
#include <algorithm>
#include "sysfsutils.h"

namespace meric {
namespace sysfsutils {

std::string MakeCounterName(const std::string &name, 
	std::map<const std::string, std::pair<std::string, std::string> > &counterLabels,
	const std::vector<std::pair<int, int>> &nums, 
	bool isLabel)
{
	auto labelIt = isLabel ? counterLabels.find(name) : counterLabels.end();

	if(labelIt == counterLabels.end())
	{
		return MakeCounterName(name, "", "", nums);
	}

	return MakeCounterName(name, labelIt->second.first, labelIt->second.second, nums);
}

std::string MakeCounterName(const std::string &name, 
	const std::string &prefix, const std::string &suffix,
	const std::vector<std::pair<int, int>> &nums)
{
	std::ostringstream oss;
	oss << (prefix.empty() ? name : prefix);

	for(const auto &num: nums)
	{
		oss << "_" << std::setfill('0') << std::setw(num.second) << num.first;
	}

	oss << (suffix.empty() ? std::string() : (" " + suffix));
	
	return oss.str();
}

std::string &RTrim(std::string &s, const char *t)
{
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}

std::string &LTrim(std::string &s, const char *t)
{
	s.erase(0, s.find_first_not_of(t));
	return s;
}

std::string &Trim(std::string &s, const char *t)
{
	return LTrim(RTrim(s, t), t);
}

std::string Trim(const std::string &s, const char *t)
{
	std::string sc = s;
	return LTrim(RTrim(sc, t), t);
}

std::string NormalizeLabel(const std::string &str)
{
	std::string out = Trim(str);
	std::replace(out.begin(), out.end(), ' ', '_');
	return out;
}

}
}
