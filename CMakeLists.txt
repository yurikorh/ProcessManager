cmake_minimum_required(VERSION 3.14)
project(ProcessManagerApp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找必要的库
find_package(Threads REQUIRED)

# 包含目录
include_directories(include)
include_directories(include/ylt/thirdparty)

# 源文件
set(SOURCES
    src/command_parser.cpp
    src/process_launcher.cpp
    src/signal_handler_new.cpp
    src/process_manager.cpp
    src/config.cpp
)

# 创建库
add_library(process_manager_lib ${SOURCES})
target_link_libraries(process_manager_lib Threads::Threads)

# 可执行文件
add_executable(process_manager src/main.cpp)
target_link_libraries(process_manager process_manager_lib Threads::Threads)

# 编译选项
target_compile_options(process_manager_lib PRIVATE -Wall -Wextra)
target_compile_options(process_manager PRIVATE -Wall -Wextra)

# 安装规则
install(TARGETS process_manager_lib process_manager
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)
