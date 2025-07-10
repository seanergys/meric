#include "msrutil.h"
#include <cstdint>
#include <fstream>
#include <string>
#include <unistd.h>
#include <map>
#include <utility>

bool meric::check_whitelist_msr(const std::vector<std::pair<uint64_t, uint64_t> > &regs)
{
	const std::vector<std::string> whitelistNames = {
		"/dev/cpu/msr_whitelist",
		"/dev/cpu/msr_allowlist"
	};

	std::string whitelistName;
	for(const std::string &name: whitelistNames)
	{
		if(access(name.c_str(), F_OK) == 0)
		{
			whitelistName = name;
			break;
		}
	}

	if(whitelistName.empty())
	{
		return false;
	}

	std::map<uint64_t, uint64_t> whitelist;

	{
		std::ifstream whitelistFile(whitelistName);
		if(!whitelistFile)
		{
			return false;
		}

		std::string comments;
		if(!std::getline(whitelistFile, comments))
		{
			return false;
		}

		uint64_t reg, mask;
		while(whitelistFile >> std::hex >> reg >> mask)
		{
			whitelist.insert(std::make_pair(reg, mask));
		}
	}

	for(const auto reg: regs)
	{
		auto it = whitelist.find(reg.first);
		if(it == whitelist.end())
		{
			return false;
		}

		if((reg.second & it->second) != reg.second)
		{
			return false;
		}
	}

	return true;
}
