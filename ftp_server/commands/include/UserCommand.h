#ifndef USERCOMMAND_H
#define USERCOMMAND_H

#include "Command.h"
#include "Session.h"

/**
 * @class UserCommand
 * @brief 处理 FTP USER 命令的类。
 *
 * UserCommand 类负责处理 FTP 中的 USER
 * 命令，用于接收客户端提供的用户名并重置登录状态。
 */
class UserCommand: public Command
{
public:
    /**
     * @brief 执行 USER 命令，接收并处理客户端提供的用户名。
     *
     * 该方法将客户端提供的用户名保存到会话中，并返回等待密码的响应。如果用户已登录，则会重置登录状态。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @param name FTP 命令名称（此处为 USER）。
     * @param params FTP 命令的参数，通常为客户端提供的用户名。
     * @param clientStream_ 与客户端通信的流。
     * @param threadPool 管理并发任务的线程池（此实现中未使用）。
     */
    void execute(
            Session& session,
            const std::string& name,
            const std::string& params,
            ACE_SOCK_Stream& clientStream_,
            ThreadPool& threadPool) override;
};

#endif // USERCOMMAND_H
