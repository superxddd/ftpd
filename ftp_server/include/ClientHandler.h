#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <ace/Event_Handler.h>
#include <ace/SOCK_Stream.h>
#include <unordered_map>
#include "ThreadPool.h"
#include "ace/Reactor.h"
#include "Session.h"
#include "FileCommand.h"
#include "Command.h"

/**
 * @class ClientHandler
 * @brief 处理客户端连接和 FTP 命令的事件处理器类。
 *
 * ClientHandler 类负责处理与客户端的通信，解析客户端发送的 FTP
 * 命令，并将其分发到相应的命令处理类。 它使用 ACE_Reactor 处理异步事件，并通过
 * ThreadPool 进行多线程任务管理。
 */
class ClientHandler: public ACE_Event_Handler
{
public:
    /**
     * @brief 构造函数，初始化 ClientHandler。
     *
     * @param clientStream 用于与客户端通信的套接字流。
     * @param reactor 指向 Reactor 的指针，用于处理事件。
     * @param threadPool 线程池，用于管理并发任务。
     */
    explicit ClientHandler(
            ACE_SOCK_Stream& clientStream,
            ACE_Reactor* reactor,
            ThreadPool& threadPool);

    /**
     * @brief 析构函数，清理资源。
     */
    ~ClientHandler() override;

    /**
     * @brief 打开客户端连接并向客户端发送 220 响应。
     *
     * @return 如果成功则返回 0，失败则返回 -1。
     */
    int open();

    /**
     * @brief 获取客户端的句柄，用于事件处理。
     *
     * @return 客户端的句柄。
     */
    virtual ACE_HANDLE get_handle() const override;

    /**
     * @brief 处理客户端输入事件。
     *
     * 该方法从客户端接收数据并解析 FTP 命令。如果客户端关闭连接，返回 -1。
     *
     * @param fd 客户端的句柄。
     * @return 如果成功则返回 0，失败则返回 -1。
     */
    virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE) override;

    /**
     * @brief 处理客户端连接关闭事件。
     *
     * 该方法在客户端关闭连接时被调用，关闭套接字并从 Reactor 中移除事件处理器。
     *
     * @param handle 关闭的句柄。
     * @param close_mask 关闭掩码。
     * @return 总是返回 0。
     */
    virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask)
            override;

    /**
     * @brief 处理异常事件，用于在 Reactor 中删除事件处理器。
     *
     * 该方法用于安全删除 `ClientHandler` 对象。
     *
     * @param fd 异常事件的句柄。
     * @return 总是返回 0。
     */
    virtual int handle_exception(ACE_HANDLE fd = ACE_INVALID_HANDLE) override;

    /**
     * @brief 处理并分发客户端命令。
     *
     * 根据客户端发送的 FTP 命令，调用相应的命令处理函数。
     *
     * @param name FTP 命令名称。
     * @param params FTP 命令的参数。
     */
    void handle_command(const std::string& name, const std::string& params);

    /**
     * @brief 获取用于与客户端通信的套接字流。
     *
     * @return 客户端的套接字流。
     */
    ACE_SOCK_Stream& getClientStream();

private:
    /**
     * @brief 解析 FTP 命令及其参数。
     *
     * @param input 输入的原始命令字符串。
     * @return 一个 pair，第一个元素为命令，第二个元素为参数。
     */
    std::pair<std::string, std::string> parse(const std::string& input);

    ACE_SOCK_Stream clientStream_; ///< 用于与客户端通信的套接字流
    Session session_; ///< 客户端会话对象，管理会话状态
    std::unordered_map<std::string, std::unique_ptr<Command> >
            commands_;        ///< FTP 命令的映射表
    FileCommand filecommand_; ///< 处理文件相关的 FTP 命令
    bool is_closed_;          ///< 标记是否已关闭连接
    ThreadPool& threadPool_;  ///< 线程池引用，用于并发任务管理
};

#endif // CLIENTHANDLER_H
