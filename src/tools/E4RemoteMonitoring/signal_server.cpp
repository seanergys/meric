#include "signal_server.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>
#include <thread> 
#include <meric.h>
#include "../../basis/utils.h"

static std::vector<std::string> splitWhitespace(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

static void runWorkloadLoop(std::atomic<bool>& sessionEnd, std::string sessionLabel) {
    MERIC_MeasureStart(sessionLabel.c_str());
    while (!sessionEnd.load()) {
        ::sleep(1);
    }
    MERIC_MeasureStop(sessionLabel.c_str());
}

int runSignalServer(uint16_t port, std::atomic<bool>& shouldStop, std::atomic<bool>& sessionEnd,bool& isMericInit) {
    const int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "socket() failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        std::cerr << "Warning: setsockopt(SO_REUSEADDR) failed: " << std::strerror(errno)
                  << "\n";
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(serverFd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0) {
        std::cerr << "bind(0.0.0.0:" << port << ") failed: " << std::strerror(errno) << "\n";
        ::close(serverFd);
        return 1;
    }

    if (listen(serverFd, 16) != 0) {
        std::cerr << "listen() failed: " << std::strerror(errno) << "\n";
        ::close(serverFd);
        return 1;
    }

    std::cout << "Listening for START/STOP on port " << port << " (Ctrl+C to quit)\n";

    std::thread worker;

    while (!shouldStop.load()) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        const int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientFd < 0) {
            if (errno == EINTR) {
                if (shouldStop.load()) {
                    break;
                }
                continue;
            }
            std::cerr << "accept() failed: " << std::strerror(errno) << "\n";
            break;
        }

        std::string payload;
        payload.reserve(32);
        char buf[256];
        while (true) {
            const ssize_t n = recv(clientFd, buf, sizeof(buf), 0);
            if (n == 0) {
                break;
            }
            if (n < 0) {
                if (errno == EINTR) {
                    if (shouldStop.load()) {
                        break;
                    }
                    continue;
                }
                std::cerr << "recv() failed: " << std::strerror(errno) << "\n";
                break;
            }
            if (payload.size() < 1024) {
                payload.append(buf, buf + n);
            }
        }
        ::close(clientFd);

        const std::string msg = meric::mutils::trimWhitespace(payload);
        if (msg.empty()) {
            continue;
        }

        const std::vector<std::string> tokens = splitWhitespace(msg);
        const std::string cmd = tokens.empty() ? std::string() : tokens[0];

        if (cmd == "START") {
            if (tokens.size() < 2) {
                std::cerr << "Invalid START message. Expected: START <computer> (got '" << msg
                          << "')\n";
                continue;
            }
            const std::string computerName = tokens[1];
            if (computerName.empty()) {
                std::cerr << "Invalid START message: missing computer name (got '" << msg << "')\n";
                continue;
            }
            if (tokens.size() > 2) {
                std::cout << "Warning: extra START tokens ignored (got '" << msg << "')\n";
            }
            else 
                std::cout << "Received START computer='" << computerName << "'\n";
            if (!worker.joinable()) {
                sessionEnd.store(false);
                if(!isMericInit) {
                    isMericInit = true;
                    ::setenv("MERIC_E4_TARGET", computerName.c_str(), 1);
                    MERIC_Init();
                }
                worker = std::thread(runWorkloadLoop, std::ref(sessionEnd), computerName);
            } else {
                std::cout << "Session already running; ignoring START\n";
            }
        } else if (cmd == "STOP") {
            if (tokens.size() >= 2) {
                std::cout << "Received STOP computer='" << tokens[1] << "'\n";
            } else {
                std::cout << "Received STOP\n";
            }
            if (worker.joinable()) {
                sessionEnd.store(true);
                worker.join();
                sessionEnd.store(false);
            }
            if(sessionEnd.load()) {
                std::cout << "Session complete\n";
                if(isMericInit) {
                    isMericInit = false;
                    MERIC_Close();
                }
            }
        } else {
            std::cout << "Received unknown message: '" << msg << "'\n";
        }
    }

    if (worker.joinable()) {
        sessionEnd.store(true);
        worker.join();
    }

    ::close(serverFd);

    return 0;
}
