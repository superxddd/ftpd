#include <gtest/gtest.h>
#include <ace/SOCK_Connector.h>
#include <ace/SOCK_Acceptor.h>
#include "Session.h"
#include "PwdCommand.h"
#include "UserCommand.h"
#include "PassCommand.h"

class PwdCommandTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 启动服务端
        ACE_INET_Addr serverAddr(12345, ACE_LOCALHOST);
        acceptor.open(serverAddr);

        // 使用 ACE_SOCK_Connector 建立客户端连接
        ACE_SOCK_Connector connector;
        connector.connect(clientStream, serverAddr);

        // 服务端接受客户端连接
        acceptor.accept(serverStream);

        session = new Session(serverStream);
        pwdCommand = new PwdCommand();
        userCommand = new UserCommand();
        passCommand = new PassCommand();

        // 模拟用户信息
        ps_map["testuser"] = "correct_password";

        // 获取当前目录
        char cwd[1024];
        ACE_OS::getcwd(cwd, sizeof(cwd));
        initialDirectory = std::string(cwd);
    }

    void TearDown() override {
        // 关闭连接
        clientStream.close();
        serverStream.close();
        acceptor.close();

        ps_map.clear();
        delete pwdCommand;
        delete userCommand;
        delete passCommand;
        delete session;
    }

    void loginUser() {
        userCommand->execute(*session, "USER", "testuser", serverStream, threadPool);
        passCommand->execute(*session, "PASS", "correct_password", serverStream, threadPool);
        ASSERT_TRUE(session->is_logged_in());
    }

    ACE_SOCK_Stream clientStream;
    ACE_SOCK_Stream serverStream;
    ACE_SOCK_Acceptor acceptor;
    ThreadPool threadPool;
    PwdCommand* pwdCommand;
    UserCommand* userCommand;
    PassCommand* passCommand;
    Session* session;
    std::string initialDirectory;
};

// 测试 PWD 命令返回正确目录
TEST_F(PwdCommandTest, TestPwdCommand) {
    loginUser();
    char nothing[256] = {0};
    clientStream.recv(nothing,sizeof(nothing));
    // 在服务端执行PWD命令
    pwdCommand->execute(*session, "PWD", "", serverStream, threadPool);

    // 在客户端接收响应
    char buffer[256] = {0};
    clientStream.recv(buffer, sizeof(buffer));

    std::string expectedResponse = "257 \"" + initialDirectory + "\" is the current directory.\r\n";

    // 验证客户端接收到的响应是否正确
    EXPECT_EQ(std::string(buffer), expectedResponse);
}
