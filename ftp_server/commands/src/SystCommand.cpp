#include "SystCommand.h"
#include <iostream>

void SystCommand::execute(
        Session& session,
        const std::string& /*name*/,
        const std::string& /*params*/,
        ACE_SOCK_Stream& clientStream_,
        ThreadPool& threadPool)
{
    if (!require_login(session)) {
        return;
    };
    std::string response;
// 根据操作系统类型返回相应的SYST响应
#if defined(__linux__)
    response = "215 UNIX Type: L8 (Linux)\r\n";
#elif defined(_WIN32)
    response = "215 Windows Type: L8\r\n";
#elif defined(__APPLE__) && defined(__MACH__)
    response = "215 UNIX Type: L8 (Mac OS)\r\n";
#else
    response = "215 UNKNOWN Type: L8\r\n";
#endif

    // 发送响应给客户端
    clientStream_.send(response.c_str(), response.size());
}