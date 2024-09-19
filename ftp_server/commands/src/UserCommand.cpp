#include "UserCommand.h"

void UserCommand::execute(
        Session& session,
        const std::string& /*name*/,
        const std::string& params,
        ACE_SOCK_Stream& clientStream_,
        ThreadPool& /*threadPool*/)
{
    session.set_logged_in(false); // 重置登录状态
    session.set_username(params); // 保存用户名

    std::string response = "331 Username okay, need password.\r\n";
    clientStream_.send(response.c_str(), response.size());
}
