#ifndef PWDCOMMAND_H
#define PWDCOMMAND_H

#include "Command.h"

/**
 * @class PwdCommand
 * @brief 处理 FTP PWD（Print Working Directory）命令的类。
 *
 * 该类负责向客户端返回当前工作目录路径，客户端可以通过该命令了解其在服务器上的当前目录。
 */
class PwdCommand: public Command
{
public:
    /**
     * @brief 执行 PWD 命令，返回当前工作目录。
     *
     * 该方法向客户端返回当前的工作目录，并以 FTP 标准响应的形式发送给客户端。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @param name FTP 命令名称（此处为 PWD）。
     * @param params FTP 命令的参数（如果提供了非空参数，将返回错误响应）。
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

#endif // PWDCOMMAND_H
