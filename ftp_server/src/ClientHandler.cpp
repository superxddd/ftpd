#include "ClientHandler.h"
#include <ace/Log_Msg.h>
#include <ace/OS_NS_unistd.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include "UserCommand.h"
#include "PassCommand.h"
#include "SystCommand.h"
#include "PwdCommand.h"
#include "FileCommand.h"
#include "CwdCommand.h"

ClientHandler::ClientHandler(
        ACE_SOCK_Stream& clientStream,
        ACE_Reactor* reactor,
        ThreadPool& threadPool)
    : clientStream_(clientStream),
      threadPool_(threadPool),
      session_(clientStream),
      is_closed_(false)
{
    this->reactor(reactor);
    commands_["USER"] = std::unique_ptr<UserCommand>(new UserCommand());
    commands_["PASS"] = std::unique_ptr<PassCommand>(new PassCommand());
    commands_["SYST"] = std::unique_ptr<SystCommand>(new SystCommand());
    commands_["PWD"] = std::unique_ptr<PwdCommand>(new PwdCommand());
    commands_["CWD"] = std::unique_ptr<CwdCommand>(new CwdCommand());
    // filecommand_ = *(new FileCommand());
}

ClientHandler::~ClientHandler()
{
    // ACE_DEBUG(
    //         (LM_DEBUG,
    //          "(Thread ID: %t) ~ClientHandler: Cleaning up resources.\n"));
}

int ClientHandler::open()
{
    // ACE_DEBUG(
    //         (LM_DEBUG, "(Thread ID: %t) Registering handler with reactor.\n"));
    // 发送 220 响应，告诉客户端服务器已准备好
    std::string response = "220 Service ready for new user.\r\n";
    ssize_t bytesSent = clientStream_.send(response.c_str(), response.size());
    if (bytesSent == -1) {
        ACE_ERROR((
                LM_ERROR, ACE_TEXT("Failed to send 220 response to client\n")));
        return -1;
    }

    return this->reactor()->register_handler(
            this, ACE_Event_Handler::READ_MASK);
}

ACE_HANDLE ClientHandler::get_handle() const
{
    return clientStream_.get_handle();
}

int ClientHandler::handle_input(ACE_HANDLE h)
{
    char buffer[4096];
    ssize_t bytesReceived = clientStream_.recv(buffer, sizeof(buffer) - 1);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::string command(buffer);
        auto [cmd, params] = parse(command);
        if (cmd.empty()) {
            return 0; // 忽略空白命令
        }
        handle_command(cmd, params);
    } else {
        return -1; // 客户端关闭连接时，返回 -1 以通知 Reactor 调用
                   // handle_close()
    }
    return 0;
}

//——————————————————关闭————————————————————————————
int ClientHandler::handle_close(
        ACE_HANDLE /*handle*/,
        ACE_Reactor_Mask /*close_mask*/)
{
    if (is_closed_) {
        return 0; // 防止多次调用 handle_close()
    }
    // ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Closing client connection.\n")));
    // 标记连接已关闭，防止重复关闭
    is_closed_ = true;
    // 关闭客户端的 socket 连接
    clientStream_.close();
    // 从 Reactor 中移除当前事件处理器
    if (this->reactor()) {
        this->reactor()->remove_handler(
                this, ACE_Event_Handler::ALL_EVENTS_MASK |
                              ACE_Event_Handler::DONT_CALL);
    }
    // 使用 ACE_Reactor::notify() 延迟释放资源，调用 handle_exception()
    ACE_Reactor::instance()->notify(this, ACE_Event_Handler::EXCEPT_MASK);
    // ACE_DEBUG(
    //         (LM_DEBUG,
    //          ACE_TEXT("(%P|%t) Safely deleting the client handler.\n")));
    clientStream_.close();
}
int ClientHandler::handle_exception(ACE_HANDLE /*fd*/)
{
    // ACE_DEBUG(
    //         (LM_DEBUG,
    //          ACE_TEXT("(%P|%t) Safely deleting the client handler.\n")));
    delete this; // 安全地删除 ClientHandler 对象
    return 0;
}
//——————————————————关闭————————————————————————————

//————————————————————命令模块————————————————————————————
// 命令分发
void ClientHandler::handle_command(
        const std::string& name,
        const std::string& params)
{
    if (name == "QUIT") {
        handle_close(ACE_INVALID_HANDLE, 0); // 处理 QUIT 命令时关闭连接
        return;
    }
    else if (name == "STOR" || name == "RETR" || name == "PASV" || name == "TYPE" ||
        name == "LIST" || name == "MKD" || name == "RMD" || name == "DELE" ||
        name == "SIZE" || name == "EPSV") {
        filecommand_.execute(
                session_, name, params, clientStream_, threadPool_);
    }
    // 检查命令是否存在于命令映射表中
    else if (commands_.find(name) != commands_.end()) {
        commands_[name]->execute(
                session_, name, params, clientStream_, threadPool_);
    } else {
        // 如果命令未被识别，则返回 500 错误响应
        std::string response = "500 Unknown command: \"" + name + "\".\r\n";
        clientStream_.send(response.c_str(), response.size());
    }
}

// 命令解析
std::pair<std::string, std::string> ClientHandler::parse(
        const std::string& input)
{
    std::string command, params;
    const auto strBegin = input.find_first_not_of(" \t\n\r");
    const auto strEnd = input.find_last_not_of(" \t\n\r");
    if (strBegin == std::string::npos) {
        return {"", ""}; // 忽略空白命令
    }
    std::string newinput = input.substr(strBegin, strEnd - strBegin + 1);
    size_t spacePos = newinput.find(' ');
    if (spacePos != std::string::npos) {
        command = newinput.substr(0, spacePos);
        params = newinput.substr(spacePos + 1);
    } else {
        command = newinput;
    }

    // 转换为大写
    for (char& c : command) {
        c = std::toupper(c);
    }

    return {command, params};
}