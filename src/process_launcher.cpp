#include "process_manager/process_launcher.h"
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include "ylt/easylog.hpp"

namespace ProcessManager {

std::optional<pid_t> ProcessLauncher::launch(const CommandArgs& args) {
    if (args.empty()) {
        return std::nullopt;
    }
    
    pid_t pid = fork();
    if (pid == 0) {
        // 子进程
        std::vector<char*> argv;
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        
        execvp(argv[0], argv.data());
        ELOG_ERROR << "Failed to exec " << argv[0];
        _exit(1);
    } else if (pid > 0) {
        // 父进程
        return pid;
    } else {
        // fork失败
        return std::nullopt;
    }
}

bool ProcessLauncher::terminate(pid_t pid, int signal) {
    return kill(pid, signal) == 0;
}

bool ProcessLauncher::isProcessAlive(pid_t pid) {
    return kill(pid, 0) == 0;
}

} // namespace ProcessManager
