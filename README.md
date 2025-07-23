# ProcessManager

一个现代化的C++进程管理器，用于启动、监控和管理多个子进程。

## 项目结构

```
ProcessManager/
├── include/process_manager/    # 头文件
│   ├── types.h                # 类型定义和枚举
│   ├── command_parser.h       # 命令行解析器
│   ├── process_launcher.h     # 进程启动器
│   ├── signal_handler.h       # 信号处理器
│   └── process_manager.h      # 主要的进程管理器
├── src/                       # 源文件
│   ├── command_parser.cpp
│   ├── process_launcher.cpp
│   ├── signal_handler.cpp
│   ├── process_manager.cpp
│   └── main.cpp
├── build/                     # 构建目录
└── CMakeLists.txt            # CMake配置
```

## 特性

- **模块化设计**: 清晰的职责分离
- **线程安全**: 使用互斥锁保护共享数据
- **自动重启**: 支持进程崩溃后自动重启
- **信号处理**: 优雅地处理进程退出和系统信号
- **状态管理**: 完整的进程生命周期管理
- **命名空间**: 避免全局命名冲突

## 编译和运行

```bash
# 创建构建目录
mkdir -p build
cd build

# 配置和编译
cmake ..
make

# 运行
./process_manager
```

## 使用示例

```cpp
#include "process_manager/process_manager.h"

int main() {
    ProcessManager::ProcessManager pm;
    
    // 添加模块
    pm.addModule("web_server", "/usr/bin/nginx", true);  // 自动重启
    pm.addModule("database", "/usr/bin/mysqld", false);  // 不自动重启
    
    // 启动模块
    pm.startModule("web_server");
    pm.startModule("database");
    
    // 检查状态
    if (pm.isRunning("web_server")) {
        std::cout << "Web server is running" << std::endl;
    }
    
    // 获取所有进程信息
    auto processes = pm.getAllProcesses();
    for (const auto& proc : processes) {
        std::cout << "Module: " << proc.name 
                  << ", PID: " << proc.pid 
                  << ", State: " << static_cast<int>(proc.state) << std::endl;
    }
    
    return 0;
}
```

## 主要改进

1. **模块化设计**: 将功能拆分为专门的类
2. **命名空间**: 使用 `ProcessManager` 命名空间
3. **线程安全**: 添加互斥锁保护共享数据
4. **错误处理**: 更好的错误检查和报告
5. **资源管理**: 自动清理和正确的生命周期管理
6. **可扩展性**: 清晰的接口便于后续扩展
7. **类型安全**: 使用枚举和结构体提高代码可读性

## API 文档

### ProcessManager 类

#### 配置管理
- `addModule(name, command, auto_restart)`: 添加新模块
- `removeModule(name)`: 移除模块

#### 进程控制
- `startModule(name)`: 启动模块
- `stopModule(name)`: 停止模块
- `restartModule(name)`: 重启模块

#### 状态查询
- `isRunning(name)`: 检查模块是否运行
- `getModuleState(name)`: 获取模块状态
- `getAllProcesses()`: 获取所有进程信息
