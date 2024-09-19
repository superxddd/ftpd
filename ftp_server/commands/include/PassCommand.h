#ifndef PASSCOMMAND_H
#define PASSCOMMAND_H

#include "Command.h"
#include "Session.h"
#include <string>
#include <map>

/// 全局用户-密码映射，用于验证用户登录
extern std::map<std::string, std::string> ps_map;

/**
 * @class PassCommand
 * @brief 处理 FTP PASS 命令的类。
 *
 * 该类负责验证 FTP 用户的密码。如果密码验证成功，用户将被标记为已登录。
 */
class PassCommand: public Command
{
public:
    /**
     * @brief 构造函数，初始化 PassCommand。
     */
    explicit PassCommand();

    /**
     * @brief 执行 PASS 命令，验证用户密码。
     *
     * 该方法从会话中获取用户名，并将客户端提供的密码与全局用户-密码映射进行匹配，
     * 如果匹配成功，则登录用户。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @param name FTP 命令名称（在此实现中未使用）。
     * @param params FTP 命令的参数，通常是用户密码。
     * @param clientStream_ 与客户端通信的流。
     * @param threadPool 管理并发任务的线程池（在此实现中未使用）。
     */
    void execute(
            Session& session,
            const std::string& name,
            const std::string& params,
            ACE_SOCK_Stream& clientStream_,
            ThreadPool& threadPool) override;

private:
    /**
     * @brief 验证用户名和密码的匹配。
     *
     * 该方法检查用户名和密码是否在全局映射中匹配，用于用户验证。
     *
     * @param username 要验证的用户名。
     * @param password 要验证的密码。
     * @return 如果用户名和密码匹配，返回 true，否则返回 false。
     */
    bool validate_user(
            const std::string& username,
            const std::string& password);
};

#endif // PASSCOMMAND_H
