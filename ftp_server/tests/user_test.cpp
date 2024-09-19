#include <gtest/gtest.h>
#include "Session.h"
#include "UserCommand.h"
#include "PassCommand.h"
#include <ace/SOCK_Stream.h>
#include <ace/OS.h>
#include <fstream>

class UserPassCommandTest : public ::testing::Test {
protected:
    void SetUp() override {
        ACE_SOCK_Stream clientStream;
        session = new Session(clientStream);
        userCommand = new UserCommand();
        passCommand = new PassCommand();

        // 模拟用户信息文件
        ps_map["testuser"] = "correct_password";
    }

    void TearDown() override {
        ps_map.clear();
        delete userCommand;
        delete passCommand;
        delete session;
    }

    void loginUser(const std::string& username, const std::string& password) {
        ACE_SOCK_Stream clientStream;
        ThreadPool threadPool;

        userCommand->execute(*session, "USER", username, clientStream, threadPool);
        passCommand->execute(*session, "PASS", password, clientStream, threadPool);
    }

    UserCommand* userCommand;
    PassCommand* passCommand;
    Session* session;
};

// 测试用户登录成功的情况
TEST_F(UserPassCommandTest, TestLoginSuccess) {
    ACE_SOCK_Stream clientStream;
    ThreadPool threadPool;

    loginUser("testuser", "correct_password");

    ASSERT_TRUE(session->is_logged_in());
}

// 测试用户登录失败的情况（用户名错误）
TEST_F(UserPassCommandTest, TestLoginFailWrongUser) {
    ACE_SOCK_Stream clientStream;
    ThreadPool threadPool;

    loginUser("wronguser", "correct_password");

    ASSERT_FALSE(session->is_logged_in());
}

// 测试用户登录失败的情况（密码错误）
TEST_F(UserPassCommandTest, TestLoginFailWrongPass) {
    ACE_SOCK_Stream clientStream;
    ThreadPool threadPool;

    loginUser("testuser", "wrong_password");

    ASSERT_FALSE(session->is_logged_in());
}
