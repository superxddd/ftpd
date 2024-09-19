#ifndef COMMAND_H
#define COMMAND_H

#include <ThreadPool.h>
#include <string>
#include <Session.h>
#include <ace/SOCK_Stream.h>
#include <ace/Log_Msg.h>

/**
 * @class Command
 * @brief FTP 命令的基类。
 *
 * Command 类作为所有 FTP 命令的基类。每个具体的 FTP 命令都需要继承该类并实现
 * `execute` 方法。
 * 它提供了一些常用的工具方法，如验证用户是否已登录以及是否处于被动模式。
 */
class Command
{
public:
    /**
     * @brief 执行 FTP 命令的抽象方法。
     *
     * 每个具体的 FTP 命令都需要实现此方法，处理来自客户端的命令请求。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @param name FTP 命令名称。
     * @param params FTP 命令的参数。
     * @param clientStream_ 用于与客户端通信的流。
     * @param threadPool 线程池，用于并发任务管理。
     */
    virtual void execute(
            Session& session,
            const std::string& name,
            const std::string& params,
            ACE_SOCK_Stream& clientStream_,
            ThreadPool& threadPool) = 0;

protected:
    /**
     * @brief 验证用户是否已登录。
     *
     * 该方法用于检查客户端是否已成功登录。如果用户未登录，则发送错误响应。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @return 如果用户已登录返回 true，否则返回 false。
     */
    bool require_login(Session& session)
    {
        if (!session.is_logged_in()) {
            std::string response = "530 Please login first.\r\n";
            session.get_client_stream().send(response.c_str(), response.size());
            return false;
        }
        return true;
    }

    /**
     * @brief 验证是否处于被动模式。
     *
     * 该方法用于检查客户端是否处于被动模式。如果未进入被动模式，则发送错误响应。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @return 如果处于被动模式返回 true，否则返回 false。
     */
    bool require_pasv_mode(Session& session)
    {
        if (!session.is_passive_mode()) {
            std::string response = "425 Use PASV first.\r\n";
            session.get_client_stream().send(response.c_str(), response.size());
            return false;
        }
        return true;
    }
};

#endif // COMMAND_H
