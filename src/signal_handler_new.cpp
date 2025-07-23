#include "process_manager/signal_handler.h"
#include <signal.h>
#include <iostream>

namespace ProcessManager {

std::atomic<bool> SignalHandler::shutdown_requested_{false};

void SignalHandler::setupShutdownHandler() {
    struct sigaction sa;
    sa.sa_handler = sigintHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // 自动重启被中断的系统调用
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

void SignalHandler::sigintHandler(int signo) {
    (void)signo;
    shutdown_requested_.store(true, std::memory_order_release);
    // 只设置原子标志，不调用任何其他函数
}

bool SignalHandler::shouldShutdown() {
    return shutdown_requested_.load(std::memory_order_acquire);
}

void SignalHandler::resetShutdownFlag() {
    shutdown_requested_.store(false, std::memory_order_release);
}

} // namespace ProcessManager
