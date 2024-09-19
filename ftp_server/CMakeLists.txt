# 指定CMake的最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目的名称和使用的C++标准
project(FTPServer VERSION 1.0 LANGUAGES CXX)

# 指定C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")

# 包含头文件目录
include_directories(${CMAKE_SOURCE_DIR}/commands/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

# 定义源文件列表（可以根据实际路径调整）
set(SOURCES
    src/main.cpp
    src/WorkerReactorTask.cpp
    src/ThreadPool.cpp
    src/MasterAcceptor.cpp
    src/ClientHandler.cpp
    src/Session.cpp
    commands/src/UserCommand.cpp
    commands/src/PassCommand.cpp
    commands/src/PwdCommand.cpp
    commands/src/CwdCommand.cpp
    commands/src/FileCommand.cpp
    commands/src/SystCommand.cpp
)

# 添加可执行文件
add_executable(ftp_server ${SOURCES})

# 链接所需的库
target_link_libraries(ftp_server ACE pthread)

add_subdirectory(tests)

