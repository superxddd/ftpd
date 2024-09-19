#include <gtest/gtest.h>
#include <ace/SOCK_Stream.h>
#include <ace/SOCK_Acceptor.h>
#include <ace/SOCK_Connector.h>
#include "ClientHandler.h"
#include "ThreadPool.h"
#include <sstream>

class ClientHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置服务器端口并初始化接受器
        ACE_INET_Addr serverAddr(12345, ACE_LOCALHOST);
        acceptor.open(serverAddr);

        // 使用 ACE_SOCK_Connector 建立客户端连接
        ACE_SOCK_Connector connector;
        connector.connect(clientStream, serverAddr);

        // 服务端接受客户端连接
        acceptor.accept(serverStream);

        // 初始化线程池
        threadPool.open(4);

        // 创建并初始化 ClientHandler
        handler = new ClientHandler(serverStream, ACE_Reactor::instance(), threadPool);

        // 注册事件处理器并发送欢迎消息
        handler->open();
    }

    void TearDown() override {
        clientStream.close();
        serverStream.close();
        acceptor.close();
        threadPool.close();

        delete handler;

    }

    void loginUser() {
        // 模拟客户端发送 USER 命令
        std::string userCommand = "USER testuser\r\n";
        clientStream.send(userCommand.c_str(), userCommand.size());

        // 接收服务器响应
        char buffer[512] = {0};
        clientStream.recv(buffer, sizeof(buffer));
        ASSERT_TRUE(std::string(buffer).find("331") != std::string::npos) << "Expected 331 response";

        // 模拟客户端发送 PASS 命令
        std::string passCommand = "PASS correct_password\r\n";
        clientStream.send(passCommand.c_str(), passCommand.size());

        // 接收服务器响应
        memset(buffer, 0, sizeof(buffer));
        clientStream.recv(buffer, sizeof(buffer));
        ASSERT_TRUE(std::string(buffer).find("230") != std::string::npos) << "Expected 230 response";
    }

    ACE_SOCK_Stream clientStream;
    ACE_SOCK_Stream serverStream;
    ACE_SOCK_Acceptor acceptor;
    ThreadPool threadPool;
    ClientHandler* handler;
};

TEST_F(ClientHandlerTest, TestInvalidCommand) {
    // 登录到服务器
    //loginUser();
    char buffer1[512] = {0};
    clientStream.recv(buffer1, sizeof(buffer1));
    // 模拟客户端发送无效指令
   // std::string invalidCommand = "INVALID_CMD\r\n";
    this->handler->handle_command("INVALID_CMD\r\n","");
    //clientStream.send(invalidCommand.c_str(), invalidCommand.size());

    // 服务端应该返回 "500 Unknown command"
    char buffer[512] = {0};
    clientStream.recv(buffer, sizeof(buffer));

    // 验证响应内容
    std::string expectedResponse = "500 Unknown command: \"INVALID_CMD\r\n\".\r\n";
    ASSERT_EQ(std::string(buffer), expectedResponse);
}
