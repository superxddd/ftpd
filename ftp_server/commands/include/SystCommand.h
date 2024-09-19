#ifndef SYSTCOMMAND_H
#define SYSTCOMMAND_H

#include "Command.h"

/**
 * @class SystCommand
 * @brief 处理 FTP SYST（系统类型）命令的类。
 *
 * 该类负责向客户端返回服务器的操作系统类型，客户端可以通过该命令了解服务器的系统类型。
 */
class SystCommand: public Command
{
public:
    /**
     * @brief 执行 SYST 命令，返回服务器的操作系统类型。
     *
     * 该方法根据服务器的操作系统类型，向客户端发送相应的系统信息。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @param name FTP 命令名称（在此实现中未使用）。
     * @param params FTP 命令的参数（在此实现中未使用）。
     * @param clientStream_ 与客户端通信的流。
     * @param threadPool 管理并发任务的线程池（在此实现中未使用）。
     */
    void execute(
            Session& session,
            const std::string& name,
            const std::string& params,
            ACE_SOCK_Stream& clientStream_,
            ThreadPool& threadPool) override;
};

#endif // SYSTCOMMAND_H
