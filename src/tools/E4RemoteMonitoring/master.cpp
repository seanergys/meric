#include <sys/stat.h>
#include <atomic>
#include <csignal>
#include <iostream>
#include <string>
#include <meric.h>
#include "ip_utils.h"
#include "signal_server.h"
#include "slave_script.h"


static std::atomic<bool> g_stop{false};
static std::atomic<bool> g_session_end{false};
bool isMericInit = false;
static void handleSignal(int) {
    g_stop.store(true);
    g_session_end.store(true);
    std::cout << "Signal received, shutting down...\n";
     if(isMericInit) {
        isMericInit = false;
        MERIC_Close();
    }
    exit(0);
}

static void printUsage(const char* prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << "            # listen for START/STOP on port 5000\n"
              << "  " << prog
              << " --init     # create slave.sh with MASTER_IP filled, then wait for START\n";
}

int main(int argc, char* argv[]) {
    bool doInit = false;
    struct stat buffer;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--init") {
            doInit = !(stat("slave.sh", &buffer) == 0);
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            printUsage(argv[0]);
            return 2;
        }
    }

    if (doInit) {
        const std::string masterIp = detectPrimaryIPv4();
        const std::string outPath = "slave.sh";
        if (!writeSlaveScript(outPath, masterIp)) {
            return 1;
        }

        std::cout << "Created '" << outPath << "' with MASTER_IP=\"" << masterIp << "\"\n";
        std::cout << "Waiting for START/STOP signals...\n";
    }

    
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    return runSignalServer(5000, g_stop, g_session_end, isMericInit);
}