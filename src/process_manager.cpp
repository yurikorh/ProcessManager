#include "process_manager/process_manager.h"
#include "process_manager/command_parser.h"
#include "process_manager/process_launcher.h"
#include "process_manager/signal_handler.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <sys/wait.h>
#include "ylt/easylog.hpp"


namespace ProcessManager {

ProcessManager::ProcessManager() {
    SignalHandler::setupShutdownHandler();
}

ProcessManager::~ProcessManager() {
    // shutdown();
}

bool ProcessManager::addModule(const std::string& name, const std::string& command, bool auto_restart) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (processes_.count(name)) {
        ELOG_ERROR << "Module [" << name << "] already exists";
        return false;
    }
    
    auto args = CommandParser::parseCommand(command);
    if (!CommandParser::validateCommand(args)) {
        ELOG_ERROR << "Invalid command for module [" << name << "]";
        return false;
    }
    
    ProcessInfo info;
    info.name = name;
    info.command = command;
    info.auto_restart = auto_restart;
    
    processes_[name] = info;
    return true;
}

bool ProcessManager::removeModule(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = processes_.find(name);
    if (it == processes_.end()) {
        return false;
    }
    
    if (it->second.state == ProcessState::RUNNING) {
        ProcessLauncher::terminate(it->second.pid, SIGTERM);
        cleanupProcess(name);
    }
    
    processes_.erase(it);
    return true;
}

bool ProcessManager::startModule(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = processes_.find(name);
    if (it == processes_.end()) {
        ELOG_ERROR << "Module [" << name << "] not found";
        return false;
    }
    
    if (it->second.state == ProcessState::RUNNING) {
        ELOG_ERROR << "Module [" << name << "] already running";
        return false;
    }
    
    auto args = CommandParser::parseCommand(it->second.command);
    auto pid = ProcessLauncher::launch(args);
    
    if (pid) {
        it->second.pid = *pid;
        it->second.state = ProcessState::RUNNING;
        pid_to_name_[*pid] = name;
        ELOG_INFO << "Started module [" << name << "] with PID " << *pid;
        return true;
    } else {
        ELOG_ERROR << "Failed to start module [" << name << "]";
        return false;
    }
}

bool ProcessManager::stopModule(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = processes_.find(name);
    if (it == processes_.end() || it->second.state != ProcessState::RUNNING) {
        return false;
    }
    
    it->second.state = ProcessState::STOPPING;
    it->second.auto_restart = false; // 防止自动重启
    ProcessLauncher::terminate(it->second.pid, SIGTERM);
    return true;
}

bool ProcessManager::restartModule(const std::string& name) {
    stopModule(name);
    
    // 等待进程真正停止
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    return startModule(name);
}

void ProcessManager::onChildExit(pid_t pid, int status) {
    std::lock_guard<std::mutex> lock(mutex_);
    ELOG_INFO << "Child process with PID " << pid << " exited with status " << status;
    
    auto pid_it = pid_to_name_.find(pid);
    if (pid_it == pid_to_name_.end()) {
        return;
    }
    
    const std::string& name = pid_it->second;
    auto proc_it = processes_.find(name);
    if (proc_it == processes_.end()) {
        return;
    }
    
    ProcessInfo& info = proc_it->second;
    ELOG_INFO << "Module [" << name << "] with PID " << pid << " exited"
              << (WIFEXITED(status) ? " with code " + std::to_string(WEXITSTATUS(status)) : 
                  WIFSIGNALED(status) ? " by signal " + std::to_string(WTERMSIG(status)) : "");
    
    bool was_stopping = (info.state == ProcessState::STOPPING);
    cleanupProcess(name);
    
    // 在shutdown过程中不重启
    if (!shutting_down_ && info.auto_restart && !was_stopping) {
        info.restart_count++;
        ELOG_INFO << "Auto-restarting module [" << name << "] (attempt " << info.restart_count << ")";
        
        // 重启逻辑：先记录需要重启的模块，在锁外处理
        restart_queue_.push_back(name);
    } else {
        ELOG_INFO << "Module [" << name << "] will not be restarted"
                  << (shutting_down_ ? " (shutting down)" : 
                      !info.auto_restart ? " (auto_restart disabled)" :
                      was_stopping ? " (was stopping)" : "");
        info.state = ProcessState::STOPPED;
    }
}

void ProcessManager::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutting_down_) {
            return; // 避免重复调用
        }
        shutting_down_ = true;
    }
    
    ELOG_INFO << "Shutting down process manager...";
    
    // 收集需要终止的进程ID
    std::vector<pid_t> pids_to_terminate;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [name, info] : processes_) {
            if (info.state == ProcessState::RUNNING && info.pid != -1) {
                ELOG_INFO << "Marking module [" << name << "] for termination";
                info.state = ProcessState::STOPPING;
                info.auto_restart = false; // 禁止自动重启
                pids_to_terminate.push_back(info.pid);
            }
        }
    }
    
    // 在锁外终止进程，避免阻塞
    for (pid_t pid : pids_to_terminate) {
        ProcessLauncher::terminate(pid, SIGTERM);
    }
    
    // 等待一小段时间后强制终止
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    for (pid_t pid : pids_to_terminate) {
        ProcessLauncher::terminate(pid, SIGKILL);
    }
    
    // 清理数据结构
    {
        std::lock_guard<std::mutex> lock(mutex_);
        processes_.clear();
        pid_to_name_.clear();
        restart_queue_.clear();
    }
    
    ELOG_INFO << "Process manager shutdown complete";
    easylog::flush();
}

bool ProcessManager::shouldExit() const {
    // 检查信号标志，但不在此处调用shutdown
    return shutting_down_ || SignalHandler::shouldShutdown();
}

void ProcessManager::processRestartQueue() {
    // 如果正在关闭，不处理重启队列
    if (shutting_down_) {
        std::lock_guard<std::mutex> lock(mutex_);
        restart_queue_.clear();
        return;
    }
    
    std::vector<std::string> to_restart;
    
    // 获取需要重启的模块列表
    {
        std::lock_guard<std::mutex> lock(mutex_);
        to_restart = std::move(restart_queue_);
        restart_queue_.clear();
    }
    
    // 在锁外重启模块
    for (const auto& name : to_restart) {
        if (shutting_down_) {
            break; // 如果在重启过程中收到shutdown信号，立即停止
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        startModule(name);
    }
}

void ProcessManager::checkChildProcesses() {
    // 如果正在关闭，不检查子进程（避免与shutdown冲突）
    if (shutting_down_) {
        return;
    }
    
    int status;
    pid_t pid;
    
    // 非阻塞地检查是否有子进程退出
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        onChildExit(pid, status);
    }
}

void ProcessManager::cleanupProcess(const std::string& name) {
    auto it = processes_.find(name);
    if (it != processes_.end()) {
        if (it->second.pid != -1) {
            pid_to_name_.erase(it->second.pid);
        }
        it->second.pid = -1;
        it->second.state = ProcessState::STOPPED;
    }
}

ProcessState ProcessManager::getModuleState(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(name);
    return it != processes_.end() ? it->second.state : ProcessState::STOPPED;
}

std::vector<ProcessInfo> ProcessManager::getAllProcesses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ProcessInfo> result;
    
    for (const auto& [name, info] : processes_) {
        result.push_back(info);
    }
    
    return result;
}

bool ProcessManager::isRunning(const std::string& name) const {
    return getModuleState(name) == ProcessState::RUNNING;
}

void ProcessManager::updateProcessState(const std::string& name, ProcessState state) {
    auto it = processes_.find(name);
    if (it != processes_.end()) {
        it->second.state = state;
    }
}

} // namespace ProcessManager
