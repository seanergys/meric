#pragma once

#include <atomic>
#include <cstdint>

// Runs a TCP server on `port` that waits for text messages START/STOP.
// - On START: starts a background workload loop (edit workload implementation in signal_server.cpp)
// - On STOP: stops the workload loop
// `shouldStop` stops the server loop; `sessionEnd` stops the current session.
// Returns 0 on clean exit, non-zero on failure.
int runSignalServer(uint16_t port, std::atomic<bool> &shouldStop, std::atomic<bool> &sessionEnd, bool &isMericInit);
