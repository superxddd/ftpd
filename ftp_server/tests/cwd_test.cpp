#include <gtest/gtest.h>
#include "Session.h"
#include "CwdCommand.h"
#include "UserCommand.h"
#include "PassCommand.h"
#include <ace/SOCK_Stream.h>
#include <ace/OS.h>
#include <fstream>

class CwdCommandTest : public ::testing::Test {
protected:
    void SetUp() override {
        ACE_SOCK_Stream clientStream;
        session = new Session(clientStream);
        cwdCommand = new CwdCommand();
        userCommand = new UserCommand();
        passCommand = new PassCommand();

        // 模拟用户信息文件
        ps_map["testuser"] = "correct_password";
        

        // 获取当前目录
        char cwd[1024];
        ACE_OS::getcwd(cwd, sizeof(cwd));
        initialDirectory = std::string(cwd);
    }

    void TearDown() override {
        ps_map.clear();

        delete cwdCommand;
        delete userCommand;
        delete passCommand;
        delete session;
    }

    void loginUser() {
        userCommand->execute(*session, "USER", "testuser", clientStream, threadPool);
        passCommand->execute(*session, "PASS", "correct_password", clientStream, threadPool);
    }

    CwdCommand* cwdCommand;
    ACE_SOCK_Stream clientStream;
    ThreadPool threadPool;
    UserCommand* userCommand;
    PassCommand* passCommand;
    Session* session;
    std::string initialDirectory;
};

// 切换到有效目录
TEST_F(CwdCommandTest, TestCwdValidDirectory) {
    loginUser();
    std::string tempDirectory = "test_cwd_dir";
    ACE_OS::mkdir(tempDirectory.c_str());

    // 将相对路径转为绝对路径
    char absoluteTempDir[1024];
    ACE_OS::realpath(tempDirectory.c_str(), absoluteTempDir);

    cwdCommand->execute(*session, "CWD", tempDirectory, clientStream, threadPool);

    char cwd[1024];
    ACE_OS::getcwd(cwd, sizeof(cwd));

    EXPECT_STREQ(cwd, absoluteTempDir);

    ACE_OS::chdir(initialDirectory.c_str());
    ACE_OS::rmdir(absoluteTempDir);
    clientStream.close();

}

// 切换到不存在的目录
TEST_F(CwdCommandTest, TestCwdNonexistentDirectory) {
    loginUser();
    std::string invalidDirectory = "nonexistent_dir";
    cwdCommand->execute(*session, "CWD", invalidDirectory, clientStream, threadPool);

    char cwd[1024];
    ACE_OS::getcwd(cwd, sizeof(cwd));

    EXPECT_STREQ(cwd, initialDirectory.c_str());
    clientStream.close();

}
