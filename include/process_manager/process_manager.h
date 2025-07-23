#pragma once
#include "types.h"
#include <unordered_map>
#include <memory>
#include <mutex>
#include "ylt/easylog.hpp"

namespace ProcessManager {

class ProcessManager {
public:
    ProcessManager();
    ~ProcessManager();

    // 配置管理
    bool addModule(const std::string& name, const std::string& command, bool auto_restart = true);
    bool removeModule(const std::string& name);
    
    // 进程控制
    bool startModule(const std::string& name);
    bool stopModule(const std::string& name);
    bool restartModule(const std::string& name);
    
    // 状态查询
    ProcessState getModuleState(const std::string& name) const;
    std::vector<ProcessInfo> getAllProcesses() const;
    bool isRunning(const std::string& name) const;
    bool shouldExit() const;
    void processRestartQueue();
    void checkChildProcesses();
    
    // 事件处理
    void onChildExit(pid_t pid, int status);
    void shutdown();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ProcessInfo> processes_;
    std::unordered_map<pid_t, std::string> pid_to_name_;
    std::vector<std::string> restart_queue_;
    bool shutting_down_ = false;
    
    void updateProcessState(const std::string& name, ProcessState state);
    void cleanupProcess(const std::string& name);
};

} // namespace ProcessManager
