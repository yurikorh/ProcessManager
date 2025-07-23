#include "process_manager/process_manager.h"
#include <iostream>
#include <thread>
#include <chrono>
#include "process_manager/config.h"
#include <numeric>

int main() {
    easylog::init_log(easylog::Severity::DEBUG, "testlog.txt", true, true);
    ProcessManager::ProcessManager pm;
    
    auto config = ProcessManager::load_config("modules.yaml");
    if (config.modules.empty()) {
        ELOG_ERROR << "No modules found in configuration.";
        return 1;
    }
    
    for (const auto& [name, module] : config.modules) {
        ELOG_INFO << "Module: " << name << ", Command: " << module.command
                  << ", Auto Restart: " << (module.restart_on_failure ? "true" : "false");
    }
    // 添加模块
    for (const auto& [name, module] : config.modules) {
        // 拼接命令行参数
        pm.addModule(name, module.command, module.restart_on_failure);
    }

    // 启动模块
    ELOG_INFO << "Starting modules...";
    for (const auto& [name, module] : config.modules) {
        pm.startModule(name);
    }
    
    // 主循环
    ELOG_INFO << "Process manager started. Press Ctrl+C to exit.";
    
    int loop_count = 0;
    
    while (!pm.shouldExit()) {
        // 检查子进程状态
        pm.checkChildProcesses();
        
        // 处理重启队列
        pm.processRestartQueue();
        
        // 每10次循环显示一次状态（约10秒）
        if (++loop_count % 10 == 0) {
            auto processes = pm.getAllProcesses();
            for (const auto& proc : processes) {
                const char* state_str = "UNKNOWN";
                switch (proc.state) {
                    case ProcessManager::ProcessState::STOPPED: state_str = "STOPPED"; break;
                    case ProcessManager::ProcessState::STARTING: state_str = "STARTING"; break;
                    case ProcessManager::ProcessState::RUNNING: state_str = "RUNNING"; break;
                    case ProcessManager::ProcessState::STOPPING: state_str = "STOPPING"; break;
                    case ProcessManager::ProcessState::CRASHED: state_str = "CRASHED"; break;
                }
                ELOG_INFO << "Module [" << proc.name << "] - State: " << state_str 
                          << ", PID: " << proc.pid << ", Restarts: " << proc.restart_count;
            }
            easylog::flush();
        }
        
        // 短暂休眠，避免过度消耗CPU
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    ELOG_INFO << "Shutdown signal received. Stopping all processes...";
    pm.shutdown();
    
    ELOG_INFO << "Exiting process manager...";

    // pm.shutdown();
    easylog::flush();
    return 0;
}
