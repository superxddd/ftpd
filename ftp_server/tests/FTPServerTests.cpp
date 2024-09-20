#include <gtest/gtest.h>
#include "FTPClient.h"
#include "FTPServer.h"
#include "TestThreadpool.h"
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/md5.h>

// 定义测试类
class FTPServerTest : public ::testing::Test {
protected:
    int port = 8888;//防止端口未复用
    void SetUp() override {
        // 启动 FTP 服务器
        ftpServer = new FTPServer(port, 4, 4);//模拟默认情况
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待服务器启动
    }

    void TearDown() override {
        port++;
        delete ftpServer;
    }

    FTPServer* ftpServer;
};

// 使用 C++ 生成大文件（1GB）
void createLargeFile(const std::string& filename, std::streamsize sizeInBytes) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Could not create file: " + filename);
    }

    const std::streamsize bufferSize = 8192;
    std::vector<char> buffer(bufferSize, 0);  // 创建一个 8KB 的缓冲区，填充为 0

    std::streamsize totalBytesWritten = 0;
    while (totalBytesWritten < sizeInBytes) {
        std::streamsize bytesToWrite = std::min(bufferSize, sizeInBytes - totalBytesWritten);
        file.write(buffer.data(), bytesToWrite);
        totalBytesWritten += bytesToWrite;
    }

    file.close();
}
// 计算文件的 MD5 哈希值,用来验证大文件传输
std::string calculateMD5(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    MD5_CTX md5Context;
    MD5_Init(&md5Context);

    char buffer[4096];
    while (file.good()) {
        file.read(buffer, sizeof(buffer));
        MD5_Update(&md5Context, buffer, file.gcount());
    }

    unsigned char result[MD5_DIGEST_LENGTH];
    MD5_Final(result, &md5Context);

    std::ostringstream hexStream;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        hexStream << std::hex << std::setw(2) << std::setfill('0') << (int)result[i];
    }

    return hexStream.str();
}



//基础
// 测试 FTP 服务器是否正确响应连接
TEST_F(FTPServerTest, Test_Connect) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();
    ASSERT_TRUE(response.find("220 Service ready for new user.") != std::string::npos);
}
//错误命令
TEST_F(FTPServerTest, Test_CommandWorng) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 测试错误命令
    response = client.sendCommand("I'msupperman\r\n");
    ASSERT_TRUE(response.find("500 Unknown command") != std::string::npos);
}

// //正确用户名密码
TEST_F(FTPServerTest, Test_LoginRight) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 测试 USER 命令
    response = client.sendCommand("USER admin\r\n");
    ASSERT_TRUE(response.find("331 Username okay") != std::string::npos);

    // 测试 PASS 命令
    response = client.sendCommand("PASS admin\r\n");
    ASSERT_TRUE(response.find("230 User logged in") != std::string::npos);
}
//错误用户名
TEST_F(FTPServerTest, Test_USERWrong) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 测试 USER 命令
    response = client.sendCommand("USER wrong\r\n");
    ASSERT_TRUE(response.find("331 Username okay, need password.") != std::string::npos);

    response = client.sendCommand("PASS admin\r\n");
    ASSERT_TRUE(response.find("530 Login incorrect") != std::string::npos);
}
//错误密码
TEST_F(FTPServerTest, Test_PASSWrong) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 测试 USER 命令
    response = client.sendCommand("USER admin\r\n");
    ASSERT_TRUE(response.find("331 Username okay, need password.") != std::string::npos);

    // 测试 PASS 命令
    response = client.sendCommand("PASS woring\r\n");
    ASSERT_TRUE(response.find("530 Login incorrect") != std::string::npos);
}


// 测试 PWD 命令
TEST_F(FTPServerTest, Test_PWD) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    char pwd[PATH_MAX];
    getcwd(pwd, sizeof(pwd));
    // 登录
    response = client.sendCommand("USER admin\r\n");
    response = client.sendCommand("PASS admin\r\n");

    // 测试 PWD 命令
    response = client.sendCommand("PWD\r\n");
    ASSERT_TRUE(response.find("257 \"" + std::string(pwd) + "\" is the current directory.\r\n") != std::string::npos);
}
// 测试 PWD带参数 命令
TEST_F(FTPServerTest, Test_PWDwithparms) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 登录
    response = client.sendCommand("USER admin\r\n");
    response = client.sendCommand("PASS admin\r\n");

    // 测试 PWD 命令
    response = client.sendCommand("PWD PPD\r\n");
    ASSERT_TRUE(response.find("500 Unknown command") != std::string::npos);
}

// 测试 DELE 命令
TEST_F(FTPServerTest, Test_DELE) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 登录
    response = client.sendCommand("USER admin\r\n");
    response = client.sendCommand("PASS admin\r\n");

    // 创建一个文件进行删除测试
    system("echo 'Test content' > testfile.txt");

    // 测试 DELE 命令
    response = client.sendCommand("DELE testfile.txt\r\n");
    ASSERT_TRUE(response.find("250 File deleted.") != std::string::npos);

    // 验证文件是否被删除
    ASSERT_NE(system("test -f testfile.txt"), 0);
}
TEST_F(FTPServerTest, Test_DELEwrong) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 登录
    response = client.sendCommand("USER admin\r\n");
    response = client.sendCommand("PASS admin\r\n");

    // 测试 DELE 命令
    response = client.sendCommand("DELE wrongfile.txt\r\n");
    ASSERT_TRUE(response.find("550 Failed to delete file.") != std::string::npos);
}

// 测试 MKD 命令
TEST_F(FTPServerTest, Test_MKD) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 登录
    response = client.sendCommand("USER admin\r\n");
    response = client.sendCommand("PASS admin\r\n");

    // 测试 MKD 命令
    response = client.sendCommand("MKD newdir\r\n");
    ASSERT_TRUE(response.find("257 Directory created") != std::string::npos);

    // 验证目录是否创建成功
    ASSERT_EQ(system("test -d newdir"), 0);

    // 清理测试目录
    system("rm -rf newdir");
}

// 测试 RMD 命令
TEST_F(FTPServerTest, Test_RMD) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 登录
    response = client.sendCommand("USER admin\r\n");
    response = client.sendCommand("PASS admin\r\n");

    // 创建一个目录进行删除测试
    system("mkdir testdir");

    // 测试 RMD 命令
    response = client.sendCommand("RMD testdir\r\n");
    ASSERT_TRUE(response.find("250 Directory deleted.") != std::string::npos);

    // 验证目录是否被删除
    ASSERT_NE(system("test -d testdir"), 0);
}

// 测试 TYPE 命令 (ASCII 模式)
TEST_F(FTPServerTest, Test_TYPEASCII) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 登录
    response = client.sendCommand("USER admin\r\n");
    response = client.sendCommand("PASS admin\r\n");

    // 设置为 ASCII 模式
    response = client.sendCommand("TYPE A\r\n");
    ASSERT_TRUE(response.find("200 Type set to A") != std::string::npos);
}

// 测试 TYPE 命令 (Binary 模式)
TEST_F(FTPServerTest, Test_TYPEBinary) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 登录
    response = client.sendCommand("USER admin\r\n");
    response = client.sendCommand("PASS admin\r\n");

    // 设置为 Binary 模式
    response = client.sendCommand("TYPE I\r\n");
    ASSERT_TRUE(response.find("200 Type set to I") != std::string::npos);
}

// 测试 TYPE 命令 (不支持的类型)
TEST_F(FTPServerTest, Test_TYPEInvalid) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 登录
    response = client.sendCommand("USER admin\r\n");
    response = client.sendCommand("PASS admin\r\n");

    // 尝试设置为不支持的类型
    response = client.sendCommand("TYPE X\r\n");
    ASSERT_TRUE(response.find("500 Unrecognized TYPE command. Supported types are I (binary) and A (ASCII).") != std::string::npos);
}

// 测试 SIZE 命令
TEST_F(FTPServerTest, Test_SIZE) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 登录
    response = client.sendCommand("USER admin\r\n");
    response = client.sendCommand("PASS admin\r\n");

    // 创建一个文件进行测试
    system("echo 'Test content' > testfile.txt");

    // 测试 SIZE 命令
    response = client.sendCommand("SIZE testfile.txt\r\n");
    ASSERT_TRUE(response.find("213 13") != std::string::npos);  // 文件大小是 13 字节

    // 清理测试文件
    system("rm -f testfile.txt");
}

// 测试 SIZE 命令 (文件不存在)
TEST_F(FTPServerTest, Test_SIZENotFound) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 登录
    response = client.sendCommand("USER admin\r\n");
    response = client.sendCommand("PASS admin\r\n");

    // 测试不存在的文件
    response = client.sendCommand("SIZE non_existing_file.txt\r\n");
    ASSERT_TRUE(response.find("550 File not found") != std::string::npos);
}


// 测试 STOR 命令 (基于 PASV 模式)
TEST_F(FTPServerTest, Test_STORPASV) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 登录
    response = client.sendCommand("USER admin\r\n");
    ASSERT_TRUE(response.find("331 Username okay") != std::string::npos);

    response = client.sendCommand("PASS admin\r\n");
    ASSERT_TRUE(response.find("230 User logged in") != std::string::npos);

    // 进入 PASV 模式
    response = client.sendCommand("PASV\r\n");
    ASSERT_TRUE(response.find("227 Entering Passive Mode") != std::string::npos);

    // 提取 PASV 返回的IP地址和端口
    int p1, p2;
    std::string::size_type start = response.find('(');
    std::string::size_type end = response.find(')', start);
    std::string pasvData = response.substr(start + 1, end - start - 1);
    int ip1, ip2, ip3, ip4;
    sscanf(pasvData.c_str(), "%d,%d,%d,%d,%d,%d", &ip1, &ip2, &ip3, &ip4, &p1, &p2);

    // 计算数据连接端口
    int dataPort = p1 * 256 + p2;
    // 建立数据连接
    FTPClient dataClient("127.0.0.1", dataPort);

    // 发送 STOR 命令准备上传文件
    response = client.sendCommand("STOR uploadfile.txt\r\n");
    ASSERT_TRUE(response.find("150 Opening data connection") != std::string::npos);

    // 上传文件内容
    const std::string fileContent = "This is the content of the file.";
    dataClient.senddata(fileContent);
    
    // 验证上传成功
    response = client.recvCommand();
    ASSERT_TRUE(response.find("226 Transfer complete") != std::string::npos);

    // 验证文件是否上传成功
    ASSERT_EQ(system("test -f uploadfile.txt"), 0);

    // 清理测试文件
    system("rm -f uploadfile.txt");
}
// 测试 STOR 命令上传大文件
TEST_F(FTPServerTest, Test_STORLargeFile) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();
    
    // 登录
    response = client.sendCommand("USER admin\r\n");
    ASSERT_TRUE(response.find("331 Username okay") != std::string::npos);
    
    response = client.sendCommand("PASS admin\r\n");
    ASSERT_TRUE(response.find("230 User logged in") != std::string::npos);
    
    // 创建大文件用于上传测试
    const std::string largeTestFile = "largefile_upload.txt";
    createLargeFile(largeTestFile, 1L * 1024 * 1024 * 1024); // 1GB 文件
    
    // 计算源文件的 MD5 哈希值
    std::string originalFileMD5 = calculateMD5(largeTestFile);
    
    // 进入 PASV 模式
    response = client.sendCommand("PASV\r\n");
    ASSERT_TRUE(response.find("227 Entering Passive Mode") != std::string::npos);
    
    // 提取 PASV 返回的 IP 地址和端口
    int p1, p2;
    std::string::size_type start = response.find('(');
    std::string::size_type end = response.find(')', start);
    std::string pasvData = response.substr(start + 1, end - start - 1);
    int ip1, ip2, ip3, ip4;
    sscanf(pasvData.c_str(), "%d,%d,%d,%d,%d,%d", &ip1, &ip2, &ip3, &ip4, &p1, &p2);
    
    // 计算数据连接端口
    int dataPort = p1 * 256 + p2;
    FTPClient dataClient("127.0.0.1", dataPort);
    
    // 发送 STOR 命令准备上传文件
    response = client.sendCommand("STOR largefile_upload.txt\r\n");
    ASSERT_TRUE(response.find("150 Opening data connection") != std::string::npos);
    
    // 上传文件内容
    std::ifstream inputFile(largeTestFile, std::ios::binary);
    ASSERT_TRUE(inputFile.is_open());
    std::ostringstream oss;
    oss << inputFile.rdbuf();
    dataClient.senddata(oss.str());
    inputFile.close();
    
    // 验证上传成功
    response = client.recvCommand();
    ASSERT_TRUE(response.find("226 Transfer complete") != std::string::npos);
    
    // 验证文件是否上传成功
    ASSERT_EQ(system("test -f largefile_upload.txt"), 0);
    
    // 计算上传后文件的 MD5 哈希值
    std::string uploadedFileMD5 = calculateMD5("largefile_upload.txt");
    
    // 比较源文件和上传文件的 MD5 哈希值
    ASSERT_EQ(originalFileMD5, uploadedFileMD5);
    
    // 清理测试文件
    system("rm -f largefile_upload.txt");
}


// 测试 RETR 命令 (基于 PASV 模式)
TEST_F(FTPServerTest, Test_RETRPASV) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 登录
    response = client.sendCommand("USER admin\r\n");
    ASSERT_TRUE(response.find("331 Username okay") != std::string::npos);

    response = client.sendCommand("PASS admin\r\n");
    ASSERT_TRUE(response.find("230 User logged in") != std::string::npos);

    // 创建一个文件用于下载测试
    const std::string testFileContent = "This is a test file.\n";
    system("echo 'This is a test file.' > testfile.txt");

    // 进入 PASV 模式
    response = client.sendCommand("PASV\r\n");
    ASSERT_TRUE(response.find("227 Entering Passive Mode") != std::string::npos);

    // 提取 PASV 返回的 IP 地址和端口
    int p1, p2;
    std::string::size_type start = response.find('(');
    std::string::size_type end = response.find(')', start);
    std::string pasvData = response.substr(start + 1, end - start - 1);
    int ip1, ip2, ip3, ip4;
    sscanf(pasvData.c_str(), "%d,%d,%d,%d,%d,%d", &ip1, &ip2, &ip3, &ip4, &p1, &p2);

    // 计算数据连接端口
    int dataPort = p1 * 256 + p2;
    FTPClient dataClient("127.0.0.1", dataPort);

    // 发送 RETR 命令请求下载文件
    response = client.sendCommand("RETR testfile.txt\r\n");
    ASSERT_TRUE(response.find("150 Opening data connection") != std::string::npos);

    // 接收文件内容
    std::string fileContent = dataClient.recvdata();
    ASSERT_EQ(fileContent, testFileContent);

    // 验证传输成功
    response = client.recvCommand();
    ASSERT_TRUE(response.find("226 Transfer complete") != std::string::npos);

    // 清理测试文件
    system("rm -f testfile.txt");
}
//RETR 命令验证大文件下载
TEST_F(FTPServerTest, Test_RETRLargeFile) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();
    // 登录
    response = client.sendCommand("USER admin\r\n");
    ASSERT_TRUE(response.find("331 Username okay") != std::string::npos);
    response = client.sendCommand("PASS admin\r\n");
    ASSERT_TRUE(response.find("230 User logged in") != std::string::npos);
    // 创建大文件用于下载测试
    const std::string largeTestFile = "largefile.txt";
    createLargeFile(largeTestFile, 1L * 1024 * 1024 * 1024); // 1GB 文件
    // 计算源文件的 MD5 哈希值
    std::string originalFileMD5 = calculateMD5(largeTestFile);
    // 进入 PASV 模式
    response = client.sendCommand("PASV\r\n");
    ASSERT_TRUE(response.find("227 Entering Passive Mode") != std::string::npos);
    // 提取 PASV 返回的 IP 地址和端口
    int p1, p2;
    std::string::size_type start = response.find('(');
    std::string::size_type end = response.find(')', start);
    std::string pasvData = response.substr(start + 1, end - start - 1);
    int ip1, ip2, ip3, ip4;
    sscanf(pasvData.c_str(), "%d,%d,%d,%d,%d,%d", &ip1, &ip2, &ip3, &ip4, &p1, &p2);
    // 计算数据连接端口
    int dataPort = p1 * 256 + p2;
    FTPClient dataClient("127.0.0.1", dataPort);
    // 发送 RETR 命令请求下载文件
    response = client.sendCommand("RETR largefile.txt\r\n");
    ASSERT_TRUE(response.find("150 Opening data connection") != std::string::npos);
    // 创建文件用于保存下载的数据
    std::ofstream outputFile("downloaded_largefile.txt", std::ios::binary);
    ASSERT_TRUE(outputFile.is_open());
    // 接收并写入数据到文件中
    std::string fileContent = dataClient.recvdata();
    outputFile.write(fileContent.c_str(), fileContent.size());
    outputFile.close();
    // 验证传输成功
    response = client.recvCommand();
    ASSERT_TRUE(response.find("226 Transfer complete") != std::string::npos);
    // 计算下载文件的 MD5 哈希值
    std::string downloadedFileMD5 = calculateMD5("downloaded_largefile.txt");
    // 比较源文件和下载文件的 MD5 哈希值
    ASSERT_EQ(originalFileMD5, downloadedFileMD5);
    // 清理测试文件
    system("rm -f largefile.txt downloaded_largefile.txt");
}


// 测试 LIST 命令 (基于 PASV 模式)
TEST_F(FTPServerTest, Test_LISTPASV) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();

    // 登录
    response = client.sendCommand("USER admin\r\n");
    ASSERT_TRUE(response.find("331 Username okay") != std::string::npos);

    response = client.sendCommand("PASS admin\r\n");
    ASSERT_TRUE(response.find("230 User logged in") != std::string::npos);

    // 创建测试文件和目录
    system("echo 'Test content' > testfile1.txt");
    system("mkdir testdir1");

    // 进入 PASV 模式
    response = client.sendCommand("PASV\r\n");
    ASSERT_TRUE(response.find("227 Entering Passive Mode") != std::string::npos);

    // 提取 PASV 返回的 IP 地址和端口
    int p1, p2;
    std::string::size_type start = response.find('(');
    std::string::size_type end = response.find(')', start);
    std::string pasvData = response.substr(start + 1, end - start - 1);
    int ip1, ip2, ip3, ip4;
    sscanf(pasvData.c_str(), "%d,%d,%d,%d,%d,%d", &ip1, &ip2, &ip3, &ip4, &p1, &p2);

    // 计算数据连接端口
    int dataPort = p1 * 256 + p2;
    FTPClient dataClient("127.0.0.1", dataPort);

    // 发送 LIST 命令请求列出当前目录内容
    response = client.sendCommand("LIST\r\n");
    ASSERT_TRUE(response.find("150 Here comes the directory listing") != std::string::npos);

    // 接收文件列表
    std::string listContent = dataClient.recvCommand();

    // 验证文件和目录是否在列表中
    ASSERT_TRUE(listContent.find("testfile1.txt") != std::string::npos);
    ASSERT_TRUE(listContent.find("testdir1") != std::string::npos);

    // 验证传输成功
    response = client.recvCommand();
    ASSERT_TRUE(response.find("226 Directory send OK") != std::string::npos);

    // 清理测试文件和目录
    system("rm -f testfile1.txt");
    system("rm -rf testdir1");
}


//性能测试
//并发测试
TEST_F(FTPServerTest, Performance_ConcurrentConnections) {
    const int num_clients = 1000;  // 并发客户端数量
    const int num_threads = 10;   // 线程池中的线程数量
    TestThreadpool thread_pool(num_threads);
    // 启动多个客户端连接到服务器
    for (int i = 0; i < num_clients; ++i) {
        thread_pool.enqueue([this]() {
            try {
                FTPClient client("127.0.0.1", port);
                std::string response = client.recvCommand();

                // 登录并执行命令
                response = client.sendCommand("USER admin\r\n");;
                response = client.sendCommand("PASS admin\r\n");

                // 执行 PWD 命令
                response = client.sendCommand("PWD\r\n");
                ASSERT_TRUE(response.find("257") != std::string::npos);

                
            } catch (const std::exception& e) {
                std::cerr << "Exception in client thread: " << e.what() << std::endl;
            }
        });
    }
}

//高频测试
TEST_F(FTPServerTest, Performance_HighFrequencyCommands) {
    FTPClient client("127.0.0.1", port);
    std::string response = client.recvCommand();
    ASSERT_TRUE(response.find("220 Service ready for new user.") != std::string::npos);

    // 登录
    response = client.sendCommand("USER admin\r\n");
    ASSERT_TRUE(response.find("331 Username okay") != std::string::npos);
    response = client.sendCommand("PASS admin\r\n");
    ASSERT_TRUE(response.find("230 User logged in") != std::string::npos);

    // 测试服务器在高频 PWD 命令下的表现
    const int num_commands = 10000;
    for (int i = 0; i < num_commands; ++i) {
        response = client.sendCommand("PWD\r\n");
        ASSERT_TRUE(response.find("257") != std::string::npos);
    }
}

//并发高频
TEST_F(FTPServerTest, Performance_ConcurrentConnectionsHighFrequency) {
    const int num_clients = 1000;  // 并发客户端数量
    const int num_threads = 10;   // 线程池中的线程数量
    TestThreadpool thread_pool(num_threads);
    // 启动多个客户端连接到服务器
    for (int i = 0; i < num_clients; ++i) {
        thread_pool.enqueue([this]() {
            try {
                FTPClient client("127.0.0.1", port);
                std::string response = client.recvCommand();

                // 登录并执行命令
                response = client.sendCommand("USER admin\r\n");;
                response = client.sendCommand("PASS admin\r\n");

                // 执行 PWD 命令
                const int num_commands = 10000;
                for (int i = 0; i < num_commands; ++i) {
                    response = client.sendCommand("PWD\r\n");
                    ASSERT_TRUE(response.find("257") != std::string::npos);
                }           
            } catch (const std::exception& e) {
                std::cerr << "Exception in client thread: " << e.what() << std::endl;
            }
        });
    }
}