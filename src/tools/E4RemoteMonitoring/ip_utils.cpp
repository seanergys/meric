#include "ip_utils.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>

std::string detectPrimaryIPv4()
{
	struct ifaddrs *ifaddr = nullptr;
	if (getifaddrs(&ifaddr) != 0)
	{
		return "127.0.0.1";
	}

	std::string best;
	for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == nullptr)
		{
			continue;
		}
		if (ifa->ifa_addr->sa_family != AF_INET)
		{
			continue;
		}
		const unsigned int flags = ifa->ifa_flags;
		if ((flags & IFF_UP) == 0)
		{
			continue;
		}
		if ((flags & IFF_LOOPBACK) != 0)
		{
			continue;
		}

		const struct sockaddr_in *sa = reinterpret_cast<const struct sockaddr_in *>(ifa->ifa_addr);
		char buf[INET_ADDRSTRLEN] = {0};
		if (inet_ntop(AF_INET, &(sa->sin_addr), buf, sizeof(buf)) == nullptr)
		{
			continue;
		}

		std::string ip(buf);
		if (ip.rfind("169.254.", 0) == 0)
		{
			continue;
		}

		best = ip;
		break;
	}

	freeifaddrs(ifaddr);
	if (best.empty())
	{
		return "127.0.0.1";
	}
	return best;
}
