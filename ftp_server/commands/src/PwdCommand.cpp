#include "PwdCommand.h"
#include <iostream>
#include <ace/OS.h>
#include <cstring>

void PwdCommand::execute(
        Session& session,
        const std::string& name,
        const std::string& params,
        ACE_SOCK_Stream& clientStream_,
        ThreadPool& /*threadPool*/)
{
    if (!require_login(session)) {
        return;
    };
    if (params.length() != 0) {
        std::string response =
                "500 Unknown command: \"" + name + params + "\".\r\n";
        clientStream_.send(response.c_str(), response.size());
    }
    // 定义缓冲区用于存储当前目录路径
    std::string pwd = session.get_working_directory();

    // 构建 FTP 响应，格式为：257 "当前目录" is the current directory.
    std::string response =
            "257 \"" + std::string(pwd) + "\" is the current directory.\r\n";

    // 将响应发送给客户端
    ssize_t bytesSent = clientStream_.send(response.c_str(), response.size());
    if (bytesSent == -1) {
        ACE_ERROR(
                (LM_ERROR,
                 ACE_TEXT("(%P|%t) Failed to send PWD response to client\n")));
    } else {
        // ACE_DEBUG(
        //         (LM_DEBUG,
        //          ACE_TEXT("(%P|%t) Sent %d bytes PWD response to client\n"),
        //          bytesSent));
    }
}