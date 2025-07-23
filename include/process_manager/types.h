#pragma once
#include <unistd.h>
#include <string>
#include <vector>

namespace ProcessManager {

enum class ProcessState {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    CRASHED
};

struct ProcessInfo {
    std::string name;
    std::string command;
    pid_t pid = -1;
    ProcessState state = ProcessState::STOPPED;
    int restart_count = 0;
    bool auto_restart = true;
};

using CommandArgs = std::vector<std::string>;

} // namespace ProcessManager
