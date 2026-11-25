#include "seanerbuswrapper.h"
#include "../basis/utils.h"
#include <future>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <vector>

#ifdef HAVE_SEANERBUS
#include <capnp/message.h>
#include <seanerbus/client/connection.hpp>
#include <seanerbus/client/simple_event_loop.hpp>

#include <seanerbus/helpers/resolve.hpp>
#include <seanerbus/msg/message.hpp>
#include <seanerbus/msg/msg.capnp.h>
#endif // HAVE_SEANERBUS

using namespace meric;

#ifdef HAVE_SEANERBUS

struct SeanerBusWrapper::NodeMap_t
{
	using Address = seanerbus::msg::Entity;

	void Clear()
	{
		coreFreq.clear();
		socketFreq.clear();
	}

	std::unordered_map<int, Address> coreFreq;
	std::unordered_map<int, std::vector<Address>> socketFreq;
};

std::thread SeanerBusWrapper::s_worker;
std::mutex SeanerBusWrapper::s_workerMutex;
std::unique_ptr<seanerbus::client::Connection> SeanerBusWrapper::s_pConnection;
std::unique_ptr<seanerbus::client::SimpleEventLoop> SeanerBusWrapper::s_pEventLoop;
std::unique_ptr<SeanerBusWrapper::NodeMap_t> SeanerBusWrapper::s_pNodeMap;

void SeanerBusWrapper::init()
{
	s_pConnection.reset(new seanerbus::client::Connection("seanerbroker://localhost:5398", "seanerbroker://localhost:5398"));
	s_pEventLoop.reset(new seanerbus::client::SimpleEventLoop(*s_pConnection));

	s_worker = std::thread([]() { s_pEventLoop->run(); });

	collectNodeInfo();

	if (s_pNodeMap)
	{
		MERIC_INFO << "Cores map: " << std::endl;
		for (const auto &[core, addr] : s_pNodeMap->coreFreq)
		{
			MERIC_INFO << core << " -> " << addr.as_string() << std::endl;
		}

		MERIC_INFO << "Socket map: " << std::endl;
		for (const auto &[socket, cores] : s_pNodeMap->socketFreq)
		{
			MERIC_INFO << "Socket: " << socket << std::endl;
			for (const auto &addr : cores)
			{
				MERIC_INFO << addr.as_string() << std::endl;
			}
		}
	}
}

void SeanerBusWrapper::close()
{
	s_pConnection->shutdown();

	if (s_worker.joinable())
	{
		s_worker.join();
	}
}

void SeanerBusWrapper::frequency(uint64_t freq, int socket)
{
	if (!s_pNodeMap)
	{
		return;
	}

	// NOTE: Set all cores since AD doesn't provide socket information
	/*auto it = s_pNodeMap->socketFreq.find(socket);
	if(it == s_pNodeMap->socketFreq.end())
		return;

	for(const auto &addr: it->second)*/
	for (auto it = s_pNodeMap->coreFreq.begin(); it != s_pNodeMap->coreFreq.end(); ++it)
	{
		seanerbus::msg::MessageBuilder<SetReqV1> bd;
		bd.builder().setValue(freq / 1000);

		s_pConnection->request(bd.to_message(it->second),
							   [](seanerbus::client::Connection &conn, seanerbus::Expected<seanerbus::msg::Message, seanerbus::Error> res) {

							   });
	}
}

void SeanerBusWrapper::uncoreFrequency(uint64_t maxFreq, uint64_t minFreq, int socket)
{
	if (!s_pNodeMap)
	{
		return;
	}
}

void SeanerBusWrapper::collectNodeInfo()
{
	std::array<char, 1024> name;
	name[name.size() - 1] = '\0';

	gethostname(name.data(), name.size() - 1);

	struct ResolveContext_t
	{
		std::unique_ptr<NodeMap_t> pNodeMap;
		size_t count;
		std::promise<void> barrier;
	};

	auto ctx = std::make_shared<ResolveContext_t>();

	seanerbus::resolve::resolve(*s_pConnection, seanerbus::Query({{"Hostname", name.data()}, {"Device", "Cpu"}, {"Metric", "Freq"}, {"Device_no", "*"}}),
								[ctx](seanerbus::client::Connection &conn, seanerbus::Expected<seanerbus::resolve::ResolveResult, seanerbus::Error> res)
								{
									if (!res.ok())
									{
										ctx->barrier.set_value();
										return;
									}

									ctx->pNodeMap = std::make_unique<NodeMap_t>();

									const auto entities = res.get_ok().entities();
									const auto resolved_names = res.get_ok().resolved_names();

									for (auto entityIt = resolved_names.cbegin(); entityIt != resolved_names.cend(); ++entityIt)
									{
										const auto namePairs = entityIt->second.name_pairs();

										if (auto it = namePairs.find("Device_no"); it != namePairs.end())
										{
											ctx->pNodeMap->coreFreq.insert(std::make_pair(std::stoi(it->second), entityIt->first));
										}

										/*if(auto it = namePairs.find("socket"); it != namePairs.end())
										{
											auto res = ctx->pNodeMap->socketFreq.insert(std::make_pair(std::stoi(it->second),
												std::vector<seanerbus::msg::Entity>()));
											res.first->second.push_back(entityIt->first);
										}*/
									}

									ctx->barrier.set_value();
								});

	auto future = ctx->barrier.get_future();
	if (auto status = future.wait_for(std::chrono::seconds(10)); status == std::future_status::ready && ctx->pNodeMap)
	{
		s_pNodeMap = std::move(ctx->pNodeMap);
	}
	else
	{
		std::cerr << "MERIC ERROR: SeanerBus request failed" << std::endl;
	}
}

#else

void SeanerBusWrapper::init() {}
void SeanerBusWrapper::close() {}

void SeanerBusWrapper::frequency(uint64_t freq, int socket) {}
void SeanerBusWrapper::uncoreFrequency(uint64_t maxFreq, uint64_t minFreq, int socket) {}

void SeanerBusWrapper::collectNodeInfo() {}

#endif // HAVE_SEANERBUS
