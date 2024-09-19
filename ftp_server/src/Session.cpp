#include "Session.h"

// 构造函数
Session::Session(ACE_SOCK_Stream& stream)
    : clientStream_(stream),
      logged_in_(false),
      passive_mode_(false),
      transfer_mode_(ASCII)
{
    // 初始化工作目录为当前用户的主目录
    working_directory_ = get_home_directory();
}

// 登录状态
bool Session::is_logged_in() const
{
    return logged_in_;
}

void Session::set_logged_in(bool logged_in)
{
    logged_in_ = logged_in;
}

// 被动模式状态
bool Session::is_passive_mode() const
{
    return passive_mode_;
}

void Session::set_passive_mode(bool passive)
{
    passive_mode_ = passive;
}

// 传输模式
TransferMode Session::get_transfer_mode() const
{
    return transfer_mode_;
}

void Session::set_transfer_mode(TransferMode mode)
{
    transfer_mode_ = mode;
}

// 工作目录
const std::string& Session::get_working_directory() const
{
    return working_directory_;
}

void Session::set_working_directory(const std::string& dir)
{
    working_directory_ = dir;
}

// 用户名
const std::string& Session::get_username() const
{
    return username_;
}

void Session::set_username(const std::string& username)
{
    this->username_ = username;
}

// 获取客户端通信流
ACE_SOCK_Stream& Session::get_client_stream()
{
    return clientStream_;
}

// 获取当前用户的主目录
std::string Session::get_home_directory()
{
    char cwd[1024];
    std::string Initdec = ACE_OS::getcwd(cwd, sizeof(cwd));
    return Initdec; // 如果失败，返回根目录
}
