# ProcessManager

一个现代化的C++进程管理器，用于启动、监控和管理多个子进程。支持YAML配置文件、自动重启、信号处理和Shell命令执行。

## 特性

✨ **主要功能**
- **YAML配置**: 通过配置文件管理模块，支持复杂的Shell命令
- **自动重启**: 进程崩溃后可自动重启
- **Shell命令支持**: 自动识别复杂Shell语法（如 `&&`, `||`, `source` 等）
- **信号处理**: 优雅处理 SIGINT/SIGTERM，确保所有子进程正确退出
- **线程安全**: 完全线程安全的设计
- **状态监控**: 实时监控所有子进程状态
- **模块化设计**: 清晰的架构和职责分离

🛡️ **稳定性特性**
- 避免信号处理器死锁
- 分阶段进程终止（SIGTERM → SIGKILL）
- 非阻塞子进程状态检查
- 安全的重启队列处理

## 项目结构

```
ProcessManager/
├── include/process_manager/    # 头文件
│   ├── types.h                # 类型定义和枚举
│   ├── command_parser.h       # 命令行解析器（支持Shell语法）
│   ├── process_launcher.h     # 进程启动器
│   ├── signal_handler.h       # 信号处理器（无死锁设计）
│   ├── process_manager.h      # 主要的进程管理器
│   └── config.h              # YAML配置解析
├── src/                       # 源文件
│   ├── command_parser.cpp
│   ├── process_launcher.cpp
│   ├── signal_handler_new.cpp
│   ├── process_manager.cpp
│   ├── config.cpp
│   └── main.cpp
├── modules.yaml              # 示例配置文件
├── build/                    # 构建目录
└── CMakeLists.txt           # CMake配置
```

## 快速开始

### 1. 编译

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

### 2. 配置文件

创建 `modules.yaml` 文件：

```yaml
modules:
  web_server:
    command: "source /etc/environment && nginx -g 'daemon off;'"
    restart_on_failure: true
    
  database:
    command: "/usr/bin/mysqld --user=mysql"
    restart_on_failure: true
    
  log_processor:
    command: "cd /var/log && python3 log_analyzer.py"
    restart_on_failure: false
```

### 3. 运行

```bash
./process_manager
```

程序将：
1. 加载 `modules.yaml` 配置
2. 启动所有定义的模块
3. 监控进程状态
4. 自动重启崩溃的进程（如果启用）
5. 响应 Ctrl+C 优雅退出

## 配置文件格式

```yaml
modules:
  模块名称:
    command: "要执行的命令"           # 必需：支持复杂Shell命令
    restart_on_failure: true/false   # 必需：是否自动重启
    depends_on: ["依赖模块"]         # 可选：依赖关系（计划功能）
    env:                            # 可选：环境变量（计划功能）
      VAR_NAME: "value"
```

### Shell命令支持

命令解析器会自动识别并处理以下Shell语法：
- `&&`, `||`, `;` 操作符
- `source` 或 `.` 命令
- 管道 `|`
- 重定向 `>`, `>>`
- 子shell `$()` 或 `` ` ` ``

这些命令会自动用 `/bin/bash -c` 包装执行。

## 编程接口

### 基本用法

```cpp
#include "process_manager/process_manager.h"
#include "process_manager/config.h"

int main() {
    // 初始化日志
    easylog::init_log(easylog::Severity::DEBUG, "process.log", true, true);
    
    // 创建进程管理器
    ProcessManager::ProcessManager pm;
    
    // 方法1：使用配置文件
    auto config = ProcessManager::load_config("modules.yaml");
    for (const auto& [name, module] : config.modules) {
        pm.addModule(name, module.command, module.restart_on_failure);
        pm.startModule(name);
    }
    
    // 方法2：手动添加模块
    pm.addModule("my_service", "python3 /path/to/service.py", true);
    pm.startModule("my_service");
    
    // 主循环
    while (!pm.shouldExit()) {
        pm.checkChildProcesses();
        pm.processRestartQueue();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // 优雅关闭
    pm.shutdown();
    return 0;
}
```

## API 文档

### ProcessManager 类

#### 配置管理
- `addModule(name, command, auto_restart)`: 添加新模块
- `removeModule(name)`: 移除模块
- `load_config(filename)`: 从YAML文件加载配置

#### 进程控制
- `startModule(name)`: 启动模块
- `stopModule(name)`: 停止模块  
- `restartModule(name)`: 重启模块
- `shutdown()`: 关闭所有模块

#### 状态查询和监控
- `isRunning(name)`: 检查模块是否运行
- `getModuleState(name)`: 获取模块状态
- `getAllProcesses()`: 获取所有进程信息
- `shouldExit()`: 检查是否应该退出
- `checkChildProcesses()`: 检查子进程状态
- `processRestartQueue()`: 处理重启队列

### 进程状态

```cpp
enum class ProcessState {
    STOPPED,    // 已停止
    STARTING,   // 启动中
    RUNNING,    // 运行中
    STOPPING,   // 停止中
    CRASHED     // 已崩溃
};
```

### 进程信息结构

```cpp
struct ProcessInfo {
    std::string name;           // 模块名称
    std::string command;        // 执行命令
    pid_t pid;                 // 进程ID
    ProcessState state;         // 当前状态
    bool auto_restart;         // 是否自动重启
    int restart_count;         // 重启次数
};
```

## 架构设计

### 核心组件

1. **ProcessManager**: 主控制器，管理所有模块的生命周期
2. **CommandParser**: 智能命令解析器，自动处理Shell语法
3. **ProcessLauncher**: 进程启动器，负责fork和exec
4. **SignalHandler**: 信号处理器，处理系统信号避免死锁
5. **Config**: 配置管理器，解析YAML配置文件

### 线程安全设计

- 使用 `std::mutex` 保护所有共享数据结构
- 信号处理器只设置原子标志，避免锁竞争
- 重启逻辑在主循环中安全执行
- 子进程状态检查使用非阻塞方式

### 死锁避免策略

1. **信号处理器简化**: 只设置原子标志，不执行复杂操作
2. **分离锁作用域**: 避免在持锁期间调用可能阻塞的函数
3. **重启队列**: 将需要重启的进程放入队列，在主循环处理
4. **非阻塞检查**: 使用 `WNOHANG` 标志避免waitpid阻塞

## 故障排除

### 常见问题

1. **程序无法启动**
   - 检查 `modules.yaml` 文件格式
   - 确保命令路径正确
   - 查看日志文件 `testlog.txt`

2. **进程无法正常退出**
   - 使用 Ctrl+C 发送SIGINT信号
   - 程序会先发送SIGTERM，500ms后发送SIGKILL

3. **Shell命令不执行**
   - 复杂Shell命令会自动用 `/bin/bash -c` 包装
   - 检查Shell语法是否正确

### 调试技巧

```bash
# 查看详细日志
tail -f testlog.txt

# 检查进程状态
ps aux | grep process_manager

# 查看配置解析
./process_manager 2>&1 | grep "Module:"
```

## 依赖

- **C++17**: 现代C++特性支持
- **YLT库**: YAML解析、日志记录、结构体反射
- **pthread**: 线程和互斥锁支持
- **POSIX系统调用**: fork, exec, waitpid, kill等

## 许可证

本项目使用MIT许可证。

## 贡献

欢迎提交问题和改进建议！

1. Fork项目
2. 创建功能分支
3. 提交更改
4. 推送到分支
5. 创建Pull Request

## 更新日志

### v2.0.0 (当前版本)
- ✅ 添加YAML配置文件支持
- ✅ 智能Shell命令解析
- ✅ 重构信号处理避免死锁
- ✅ 改进进程重启逻辑
- ✅ 添加详细日志记录

### v1.0.0
- ✅ 基础进程管理功能
- ✅ 自动重启支持
- ✅ 线程安全设计
- ✅ 模块化架构
