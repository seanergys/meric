#pragma once

#ifndef SRC_WRAPPER_SEANERBUSWRAPPER_H_
#define SRC_WRAPPER_SEANERBUSWRAPPER_H_

#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace seanerbus
{
namespace client
{
class Connection;
class SimpleEventLoop;
} // namespace client
} // namespace seanerbus

namespace meric
{

class SeanerBusWrapper
{
public:
	static void init();
	static void close();

	static void frequency(uint64_t freq, int socket = -1);
	static void uncoreFrequency(uint64_t maxFreq, uint64_t minFreq, int socket = -1);

protected:
	struct NodeMap_t;

	static void collectNodeInfo();

#ifdef HAVE_SEANERBUS
	static std::thread s_worker;
	static std::mutex s_workerMutex;
	static std::unique_ptr<seanerbus::client::Connection> s_pConnection;
	static std::unique_ptr<seanerbus::client::SimpleEventLoop> s_pEventLoop;
	static std::unique_ptr<NodeMap_t> s_pNodeMap;
#endif // HAVE_SEANERBUS
};

} // namespace meric

#endif // SRC_WRAPPER_SEANERBUSWRAPPER_H_
