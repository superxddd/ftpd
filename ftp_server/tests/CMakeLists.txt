# tests/CMakeLists.txt

# 设置最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称
project(FTPServerTests)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 找到 GTest 和 GMock
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})


# 包含项目头文件目录
include_directories(${PROJECT_SOURCE_DIR}/../include)
include_directories(${PROJECT_SOURCE_DIR}/../commands/include)

# 定义项目源文件列表（添加到测试可执行文件中）
set(PROJECT_SOURCES
    ${PROJECT_SOURCE_DIR}/../src/WorkerReactorTask.cpp
    ${PROJECT_SOURCE_DIR}/../src/ThreadPool.cpp
    ${PROJECT_SOURCE_DIR}/../src/MasterAcceptor.cpp
    ${PROJECT_SOURCE_DIR}/../src/ClientHandler.cpp
    ${PROJECT_SOURCE_DIR}/../src/Session.cpp
    ${PROJECT_SOURCE_DIR}/../commands/src/UserCommand.cpp
    ${PROJECT_SOURCE_DIR}/../commands/src/PassCommand.cpp
    ${PROJECT_SOURCE_DIR}/../commands/src/PwdCommand.cpp
    ${PROJECT_SOURCE_DIR}/../commands/src/CwdCommand.cpp
    ${PROJECT_SOURCE_DIR}/../commands/src/FileCommand.cpp
    ${PROJECT_SOURCE_DIR}/../commands/src/SystCommand.cpp
)

# 添加测试源文件
set(TEST_SOURCES
    ${PROJECT_SOURCE_DIR}/c_main.cpp
    ${PROJECT_SOURCE_DIR}/pwd_test.cpp
    ${PROJECT_SOURCE_DIR}/user_test.cpp
    ${PROJECT_SOURCE_DIR}/cwd_test.cpp
    ${PROJECT_SOURCE_DIR}/file_test.cpp
    ${PROJECT_SOURCE_DIR}/data_test.cpp
    ${PROJECT_SOURCE_DIR}/Unknowcommand_test
    )


# 添加测试可执行文件，包含测试和项目源文件
add_executable(ftp_server_test_suite ${TEST_SOURCES} ${PROJECT_SOURCES})

# 链接 GoogleTest 库和 pthread 库
target_link_libraries(ftp_server_test_suite GTest::gtest_main ACE GTest::gmock pthread)

# 启用测试
enable_testing()

# 添加测试命令
add_test(NAME ftp_server_test_suite COMMAND ftp_server_test_suite)
