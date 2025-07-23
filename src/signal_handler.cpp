#include "process_manager/signal_handler.h"
#include <signal.h>
#include <sys/wait.h>
#include <iostream>
#include "ylt/easylog.hpp"

namespace ProcessManager {

SignalHandler::ChildExitCallback SignalHandler::child_callback_;
std::function<void()> SignalHandler::shutdown_callback_;

void SignalHandler::setupChildHandler(ChildExitCallback callback) {
    child_callback_ = callback;
    
    struct sigaction sa;
    sa.sa_handler = sigchldHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, nullptr);
}

void SignalHandler::setupShutdownHandler(std::function<void()> callback) {
    shutdown_callback_ = callback;
    
    struct sigaction sa;
    sa.sa_handler = sigintHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // 移除 SA_RESTART 标志，让信号能够中断睡眠
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

void SignalHandler::sigchldHandler(int signo) {
    (void)signo; // 抑制未使用参数警告
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (child_callback_) {
            child_callback_(pid, status);
        }
    }
}

void SignalHandler::sigintHandler(int signo) {
    (void)signo; // 抑制未使用参数警告
    if (shutdown_callback_) {
        shutdown_callback_();
    } else {
        ELOG_ERROR << "No shutdown callback registered!";
    }
}

} // namespace ProcessManager
