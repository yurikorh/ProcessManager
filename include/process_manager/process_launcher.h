#pragma once
#include "types.h"
#include <optional>
#include <signal.h>

namespace ProcessManager {

class ProcessLauncher {
public:
    static std::optional<pid_t> launch(const CommandArgs& args);
    static bool terminate(pid_t pid, int signal = SIGTERM);
    static bool isProcessAlive(pid_t pid);
};

} // namespace ProcessManager
