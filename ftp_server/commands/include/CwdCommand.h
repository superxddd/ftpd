#ifndef CWDCOMMAND_H
#define CWDCOMMAND_H

#include "Command.h"
#include <string>

/**
 * @class CwdCommand
 * @brief 处理 FTP CWD（更改工作目录）命令的类。
 *
 * CwdCommand 类负责实现 FTP 协议中的 CWD 命令。该命令用于更改客户端的工作目录。
 * 该类会检查提供的路径是否有效，并更新会话的工作目录。如果路径无效，向客户端发送错误响应。
 */
class CwdCommand: public Command
{
public:
    /**
     * @brief 执行 CWD 命令。
     *
     * 根据客户端提供的路径参数，尝试更改会话的当前工作目录。
     * 如果路径无效或用户没有登录，向客户端发送相应的错误信息。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @param name FTP 命令的名称（在此实现中未使用）。
     * @param params FTP 命令的参数，通常是目标路径。
     * @param clientStream_ 用于与客户端通信的套接字流。
     * @param threadPool 线程池，用于管理并发任务（在此实现中未使用）。
     */
    void execute(
            Session& session,
            const std::string& name,
            const std::string& params,
            ACE_SOCK_Stream& clientStream_,
            ThreadPool& threadPool) override;

private:
    /**
     * @brief 展开波浪符 (~) 为用户的主目录。
     *
     * 该方法负责将路径中的波浪符 `~` 展开为用户的主目录，例如 `~/documents`
     * 展开为 `/home/username/documents`。
     * 如果路径中不包含波浪符，则返回原始路径。
     *
     * @param path 要展开的路径字符串。
     * @return 展开后的完整路径。如果路径中没有波浪符，返回原始路径。
     */
    std::string expand_tilde(const std::string& path);
};

#endif // CWDCOMMAND_H
