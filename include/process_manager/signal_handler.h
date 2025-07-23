#pragma once
#include <functional>
#include <unistd.h>
#include <atomic>

namespace ProcessManager {

class SignalHandler {
public:
    static void setupShutdownHandler();
    static bool shouldShutdown();
    static void resetShutdownFlag();
    
private:
    static std::atomic<bool> shutdown_requested_;
    static void sigintHandler(int signo);
};

} // namespace ProcessManager
