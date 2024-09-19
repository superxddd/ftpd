#include "PassCommand.h"
#include <fstream>
#include <sstream>

PassCommand::PassCommand() {}

void PassCommand::execute(
        Session& session,
        const std::string& /*name*/,
        const std::string& params,
        ACE_SOCK_Stream& clientStream_,
        ThreadPool& /*threadPool*/)
{
    std::string username = session.get_username(); // 获取当前会话中的用户名
    std::string password = params;           // 获取客户端传入的密码
    if (validate_user(username, password)) { // 验证用户凭证
        session.set_logged_in(true); // 如果验证成功，将用户标记为已登录
        std::string response = "230 User logged in, proceed.\r\n";
        clientStream_.send(response.c_str(), response.size()); // 发送成功响应
    } else {
        std::string response = "530 Login incorrect.\r\n";
        clientStream_.send(response.c_str(), response.size()); // 发送失败响应
    }
}

bool PassCommand::validate_user(
        const std::string& username,
        const std::string& password)
{
    // 遍历全局用户-密码映射，检查是否有匹配的用户名和密码
    for (auto it = ps_map.begin(); it != ps_map.end(); ++it) {
        if (it->first == username && it->second == password) {
            return true;
        }
    }
    return false; // 如果没有匹配的用户，返回 false
}
