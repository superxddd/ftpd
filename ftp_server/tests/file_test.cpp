#include <gtest/gtest.h>
#include "Session.h"
#include "FileCommand.h"
#include "UserCommand.h"
#include "PassCommand.h"
#include <ace/SOCK_Stream.h>
#include <ace/OS.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

class FileCommandTest : public ::testing::Test {
protected:
    void SetUp() override {
        ACE_SOCK_Stream clientStream;
        session = new Session(clientStream);
        fileCommand = new FileCommand();
        userCommand = new UserCommand();
        passCommand = new PassCommand();

        // 模拟用户信息文件
        ps_map["testuser"] = "correct_password";

        char cwd[1024];
        ACE_OS::getcwd(cwd, sizeof(cwd));
        initialDirectory = std::string(cwd);
    }

    void TearDown() override {
        ps_map.clear();

        delete fileCommand;
        delete userCommand;
        delete passCommand;
        delete session;
    }

    void loginUser() {


        userCommand->execute(*session, "USER", "testuser", clientStream, threadPool);
        passCommand->execute(*session, "PASS", "correct_password", clientStream, threadPool);
    }

    FileCommand* fileCommand;
    UserCommand* userCommand;
    PassCommand* passCommand;
    Session* session;
    ACE_SOCK_Stream clientStream;
    ThreadPool threadPool;
    std::string initialDirectory;
};

// 测试 MKD 命令
TEST_F(FileCommandTest, TestMkdCommand) {
    loginUser();
    std::string newDirectory = "test_mkd_dir";
    fileCommand->execute(*session, "MKD", newDirectory, clientStream, threadPool);

    EXPECT_TRUE(ACE_OS::access(newDirectory.c_str(), F_OK) == 0);

    // 清理创建的目录
    ACE_OS::rmdir(newDirectory.c_str());
    clientStream.close();
}

// 测试 RMD 命令
TEST_F(FileCommandTest, TestRmdCommand) {
    loginUser();
    threadPool.open(4);

    std::string newDirectory = "test_rmd_dir";
    ACE_OS::mkdir(newDirectory.c_str());

    fileCommand->execute(*session, "RMD", newDirectory, clientStream, threadPool);

    EXPECT_FALSE(ACE_OS::access(newDirectory.c_str(), F_OK) == 0);
    clientStream.close();
}

// 测试 DELE 命令
TEST_F(FileCommandTest, TestDeleCommand) {
    loginUser();
;

    std::string newFile = "test_file.txt";
    std::ofstream outFile(newFile);
    outFile << "Test content";
    outFile.close();

    fileCommand->execute(*session, "DELE", newFile, clientStream, threadPool);

    EXPECT_FALSE(ACE_OS::access(newFile.c_str(), F_OK) == 0);
    clientStream.close();
}

// 测试 SIZE 命令
TEST_F(FileCommandTest, TestSizeCommand) {
    loginUser();


    std::string newFile = "test_file.txt";
    std::ofstream outFile(newFile);
    outFile << "Test content";
    outFile.close();

    fileCommand->execute(*session, "SIZE", newFile, clientStream, threadPool);

    struct stat fileStat;
    if (stat(newFile.c_str(), &fileStat) == 0) {
        EXPECT_TRUE(fileStat.st_size > 0);
    } else {
        FAIL() << "File size check failed: Unable to stat file";
    }

    // 清理创建的文件
    std::remove(newFile.c_str());
    clientStream.close();
}

// 测试 TYPE 命令
TEST_F(FileCommandTest, TestTypeCommand) {
    loginUser();


    fileCommand->execute(*session, "TYPE", "I", clientStream, threadPool);
    EXPECT_EQ(session->get_transfer_mode(), BINARY);

    fileCommand->execute(*session, "TYPE", "A", clientStream, threadPool);
    EXPECT_EQ(session->get_transfer_mode(), ASCII);
    clientStream.close();
}

// 测试 MKD 已存在目录的情况
TEST_F(FileCommandTest, TestMkdExistingDirectory) {
    loginUser();


    std::string newDirectory = "test_mkd_existing_dir";
    ACE_OS::mkdir(newDirectory.c_str());

    fileCommand->execute(*session, "MKD", newDirectory, clientStream, threadPool);

    // 尝试再次创建相同目录，应该成功返回目录已存在
    EXPECT_TRUE(ACE_OS::access(newDirectory.c_str(), F_OK) == 0);

    // 清理创建的目录
    ACE_OS::rmdir(newDirectory.c_str());
    clientStream.close();
}

// 测试 RMD 删除不存在目录
TEST_F(FileCommandTest, TestRmdNonExistentDirectory) {
    loginUser();


    std::string nonExistentDirectory = "non_existent_dir";
    fileCommand->execute(*session, "RMD", nonExistentDirectory, clientStream, threadPool);

    // 验证目录不存在
    EXPECT_FALSE(ACE_OS::access(nonExistentDirectory.c_str(), F_OK) == 0);

    clientStream.close();
}

// 测试 DELE 删除不存在文件
TEST_F(FileCommandTest, TestDeleNonExistentFile) {
    loginUser();

    std::string nonExistentFile = "non_existent_file.txt";
    fileCommand->execute(*session, "DELE", nonExistentFile, clientStream, threadPool);

    // 验证文件不存在
    EXPECT_FALSE(ACE_OS::access(nonExistentFile.c_str(), F_OK) == 0);

    clientStream.close();
}

// 测试 SIZE 获取不存在文件大小
TEST_F(FileCommandTest, TestSizeNonExistentFile) {
    loginUser();


    std::string nonExistentFile = "non_existent_file.txt";
    fileCommand->execute(*session, "SIZE", nonExistentFile, clientStream, threadPool);

    // 验证文件不存在，应该无法获取文件大小
    struct stat fileStat;
    EXPECT_FALSE(stat(nonExistentFile.c_str(), &fileStat) == 0);

    clientStream.close();
}

// 测试 TYPE 处理无效的传输类型
TEST_F(FileCommandTest, TestInvalidTypeCommand) {
    loginUser();

    fileCommand->execute(*session, "TYPE", "Z", clientStream, threadPool);  // 无效的传输类型
    EXPECT_NE(session->get_transfer_mode(), BINARY);  // 确保没有切换到 BINARY

    fileCommand->execute(*session, "TYPE", "X", clientStream, threadPool);  // 另一个无效的传输类型
    EXPECT_NE(session->get_transfer_mode(), BINARY);  // 确保没有切换到 BINARY

    clientStream.close();
}

// 测试 TYPE 命令设置有效类型
TEST_F(FileCommandTest, TestValidTypeCommand) {
    loginUser();


    fileCommand->execute(*session, "TYPE", "I", clientStream, threadPool);  // 二进制模式
    EXPECT_EQ(session->get_transfer_mode(), BINARY);

    fileCommand->execute(*session, "TYPE", "A", clientStream, threadPool);  // ASCII 模式
    EXPECT_EQ(session->get_transfer_mode(), ASCII);

    clientStream.close();
}
