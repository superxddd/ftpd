#include <gtest/gtest.h>
#include <ace/SOCK_Connector.h>
#include <ace/SOCK_Acceptor.h>
#include "Session.h"
#include "FileCommand.h"
#include "UserCommand.h"
#include "PassCommand.h"
#include <fstream>

class DataCommandTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 启动服务端，监听控制连接
        ACE_INET_Addr serverAddr(12345, ACE_LOCALHOST);
        acceptor.open(serverAddr);

        // 使用 ACE_SOCK_Connector 建立客户端控制连接
        ACE_SOCK_Connector connector;
        connector.connect(controlClientStream, serverAddr);

        // 服务端接受客户端控制连接
        acceptor.accept(controlServerStream);

        session = new Session(controlServerStream);
        fileCommand = new FileCommand();
        userCommand = new UserCommand();
        passCommand = new PassCommand();

        // 启动线程池
        threadPool.open(4);

        // 模拟用户信息
        ps_map["testuser"] = "correct_password";

        // 创建一个文件供测试 RETR 命令
        std::ofstream testFile("testfile.txt");
        testFile << "Test file content.";
        testFile.close();
    }

    void TearDown() override
    {
        controlClientStream.close();
        controlServerStream.close();
        dataClientStream.close();
        acceptor.close();

        ps_map.clear();
        delete fileCommand;
        delete userCommand;
        delete passCommand;
        delete session;

        std::remove("testfile.txt");

        threadPool.close();
    }

    void loginUser()
    {
        userCommand->execute(
                *session, "USER", "testuser", controlServerStream, threadPool);
        passCommand->execute(
                *session, "PASS", "correct_password", controlServerStream,
                threadPool);
        ASSERT_TRUE(session->is_logged_in());
    }

    ACE_SOCK_Stream controlClientStream;
    ACE_SOCK_Stream controlServerStream;
    ACE_SOCK_Stream dataClientStream;
    ACE_SOCK_Acceptor acceptor;
    ThreadPool threadPool;
    FileCommand* fileCommand;
    UserCommand* userCommand;
    PassCommand* passCommand;
    Session* session;
};

TEST_F(DataCommandTest, TestRetrCommandInPasvMode)
{
    loginUser();

    // 服务器进入 PASV 模式，获取服务器的端口
    fileCommand->handle_pasv(*session, controlServerStream);

    // 从服务器的响应中解析出 IP 地址和端口
    char response[512] = {0};
    controlClientStream.recv(response, sizeof(response));

    // 查找 "227 Entering Passive Mode" 开始的位置
    char* pasvStart = strstr(response, "227 Entering Passive Mode");
    ASSERT_NE(pasvStart, nullptr)
            << "Failed to find PASV response in server output.";

    int h1, h2, h3, h4, p1, p2;
    ASSERT_EQ(
            sscanf(pasvStart, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
                   &h1, &h2, &h3, &h4, &p1, &p2),
            6)
            << "Failed to parse PASV response.";

    int port = (p1 * 256) + p2;

    // 使用解析出的端口，客户端建立数据连接
    ACE_INET_Addr dataAddr(port, "127.0.0.1");
    ACE_SOCK_Connector dataConnector;
    ASSERT_NE(dataConnector.connect(dataClientStream, dataAddr), -1)
            << "Failed to connect to PASV port.";

    // 服务器执行 RETR 命令，传输文件
    fileCommand->handle_retr(
            *session, "testfile.txt", controlServerStream, threadPool);

    // 在客户端接收文件内容
    char buffer[256] = {0};
    ssize_t bytesReceived = dataClientStream.recv(buffer, sizeof(buffer));
    ASSERT_GT(bytesReceived, 0) << "Failed to receive file content.";

    // 验证接收到的文件内容
    std::string expectedContent = "Test file content.\r\n";
    EXPECT_EQ(std::string(buffer, bytesReceived), expectedContent);

    dataClientStream.close();
    fileCommand->dataStream_.close();
}

TEST_F(DataCommandTest, TestListCommandInPasvMode)
{
    loginUser();

    // 服务器进入 PASV 模式，获取服务器的端口
    fileCommand->handle_pasv(*session, controlServerStream);

    // 从服务器的响应中解析出 IP 地址和端口
    char response[512] = {0};
    controlClientStream.recv(response, sizeof(response));

    // 查找 "227 Entering Passive Mode" 开始的位置
    char* pasvStart = strstr(response, "227 Entering Passive Mode");
    ASSERT_NE(pasvStart, nullptr)
            << "Failed to find PASV response in server output.";

    int h1, h2, h3, h4, p1, p2;
    ASSERT_EQ(
            sscanf(pasvStart, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
                   &h1, &h2, &h3, &h4, &p1, &p2),
            6)
            << "Failed to parse PASV response.";

    int port = (p1 * 256) + p2;

    // 使用解析出的端口，客户端建立数据连接
    ACE_INET_Addr dataAddr(port, "127.0.0.1");
    ACE_SOCK_Connector dataConnector;
    ASSERT_NE(dataConnector.connect(dataClientStream, dataAddr), -1)
            << "Failed to connect to PASV port.";

    // 服务器执行 LIST 命令，列出目录内容
    fileCommand->handle_list(*session, controlServerStream, threadPool);

    // 在客户端接收目录列表
    char buffer[1024] = {0};
    ssize_t bytesReceived = dataClientStream.recv(buffer, sizeof(buffer));
    ASSERT_GT(bytesReceived, 0) << "Failed to receive directory listing.";

    // 验证接收到的目录列表包含文件信息
    std::string expectedContent = "testfile.txt";
    EXPECT_NE(
            std::string(buffer, bytesReceived).find(expectedContent),
            std::string::npos);

    dataClientStream.close();
    fileCommand->dataStream_.close();
}

TEST_F(DataCommandTest, TestStorCommandInPasvMode)
{
    loginUser();

    // 服务器进入 PASV 模式，获取服务器的端口
    fileCommand->handle_pasv(*session, controlServerStream);

    // 从服务器的响应中解析出 IP 地址和端口
    char response[512] = {0};
    controlClientStream.recv(response, sizeof(response));

    // 查找 "227 Entering Passive Mode" 开始的位置
    char* pasvStart = strstr(response, "227 Entering Passive Mode");
    ASSERT_NE(pasvStart, nullptr)
            << "Failed to find PASV response in server output.";

    int h1, h2, h3, h4, p1, p2;
    ASSERT_EQ(
            sscanf(pasvStart, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
                   &h1, &h2, &h3, &h4, &p1, &p2),
            6)
            << "Failed to parse PASV response.";

    int port = (p1 * 256) + p2;

    // 使用解析出的端口，客户端建立数据连接
    ACE_INET_Addr dataAddr(port, "127.0.0.1");
    ACE_SOCK_Connector dataConnector;
    ASSERT_NE(dataConnector.connect(dataClientStream, dataAddr), -1)
            << "Failed to connect to PASV port.";

    // 服务器执行 STOR 命令，准备接收文件
    fileCommand->handle_stor(
            *session, "uploaded_file.txt", controlServerStream, threadPool);

    // 客户端发送文件内容
    std::string fileContent = "Uploaded file content.\r\n";
    ssize_t bytesSent =
            dataClientStream.send(fileContent.c_str(), fileContent.size());
    ASSERT_GT(bytesSent, 0) << "Failed to send file content.";
    dataClientStream.close(); // 关闭数据流

    // 等待文件写入完成
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 检查服务器是否正确保存了文件
    std::ifstream uploadedFile("uploaded_file.txt", std::ios::binary);
    ASSERT_TRUE(uploadedFile.is_open()) << "Failed to open uploaded file.";

    // 获取文件大小
    uploadedFile.seekg(0, std::ios::end);
    std::streampos fileSize = uploadedFile.tellg();
    uploadedFile.seekg(0, std::ios::beg);

    // 逐字节读取文件内容
    std::vector<char> fileContentBytes(fileSize);
    uploadedFile.read(fileContentBytes.data(), fileSize);
    std::string fileContentStr(
            fileContentBytes.begin(), fileContentBytes.end());

    ASSERT_EQ(fileContentStr, fileContent);

    uploadedFile.close();
    std::remove("uploaded_file.txt"); // 删除测试文件
}
