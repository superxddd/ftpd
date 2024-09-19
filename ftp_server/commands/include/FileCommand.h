#ifndef FILECOMMAND_H
#define FILECOMMAND_H

#include "Command.h"
#include "Session.h"
#include "ThreadPool.h"
#include <ace/SOCK_Acceptor.h>
#include <ace/SOCK_Stream.h>

/**
 * @class FileCommand
 * @brief 处理 FTP 文件传输相关命令的类。
 *
 * FileCommand 类负责处理文件操作相关的 FTP 命令，如 PASV、STOR、RETR、LIST 等。
 * 它支持被动模式和各种传输模式（如 ASCII
 * 和二进制模式），并使用线程池来管理并发任务。
 */
class FileCommand: public Command
{
public:
    /**
     * @brief 构造函数，初始化数据连接的套接字。
     */
    explicit FileCommand();

    /**
     * @brief 执行给定的 FTP 命令。
     *
     * 根据传入的命令名称选择适当的处理函数来执行相应的操作。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @param name FTP 命令名称（如 PASV、STOR、RETR 等）。
     * @param params 命令的参数，如文件路径或传输类型。
     * @param clientStream_ 与客户端通信的流。
     * @param threadPool 管理并发任务的线程池。
     */
    void execute(
            Session& session,
            const std::string& name,
            const std::string& params,
            ACE_SOCK_Stream& clientStream_,
            ThreadPool& threadPool) override;

    /**
     * @brief 处理 PASV 命令，进入被动模式。
     *
     * 该方法设置被动模式，随机选择端口并监听来自客户端的数据连接请求。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @param clientStream_ 与客户端通信的流。
     */
    void handle_pasv(Session& session, ACE_SOCK_Stream& clientStream_);

    /**
     * @brief 处理 TYPE 命令，设置传输模式。
     *
     * 根据客户端请求，设置数据传输模式为 ASCII 或 Binary。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @param params FTP 命令的参数，指定传输模式（如 "A" 表示 ASCII，"I" 表示
     * Binary）。
     * @param clientStream_ 与客户端通信的流。
     */
    void handle_type(
            Session& session,
            const std::string& params,
            ACE_SOCK_Stream& clientStream_);

    /**
     * @brief 处理 STOR 命令，将客户端上传的文件存储在服务器上。
     *
     * 该方法通过线程池处理文件上传操作，并根据传输模式（ASCII 或
     * Binary）进行处理。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @param params FTP 命令的参数，指定存储文件的路径。
     * @param clientStream_ 与客户端通信的流。
     * @param threadPool 管理并发任务的线程池。
     */
    void handle_stor(
            Session& session,
            const std::string& params,
            ACE_SOCK_Stream& clientStream_,
            ThreadPool& threadPool);

    /**
     * @brief 处理 RETR 命令，将服务器上的文件发送到客户端。
     *
     * 该方法通过线程池处理文件下载操作，并根据传输模式（ASCII 或
     * Binary）进行处理。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @param params FTP 命令的参数，指定要下载的文件路径。
     * @param clientStream_ 与客户端通信的流。
     * @param threadPool 管理并发任务的线程池。
     */
    void handle_retr(
            Session& session,
            const std::string& params,
            ACE_SOCK_Stream& clientStream_,
            ThreadPool& threadPool);

    /**
     * @brief 处理 LIST 命令，列出服务器端当前目录的文件和目录。
     *
     * 该方法通过线程池处理 LIST 命令，列出当前工作目录的文件和子目录。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @param clientStream_ 与客户端通信的流。
     * @param threadPool 管理并发任务的线程池。
     */
    void handle_list(
            Session& session,
            ACE_SOCK_Stream& clientStream_,
            ThreadPool& threadPool);

    /**
     * @brief 处理 MKD 命令，在服务器端创建新目录。
     *
     * @param session 当前 FTP 客户端会话状态。
     * @param params FTP 命令的参数，指定要创建的目录路径。
     * @param clientStream_ 与客户端通信的流。
     */
    void handle_mkd(
            Session& session,
            const std::string& params,
            ACE_SOCK_Stream& clientStream_);

    /**
     * @brief 处理 RMD 命令，删除服务器端的目录。
     *
     * @param params FTP 命令的参数，指定要删除的目录路径。
     * @param clientStream_ 与客户端通信的流。
     */
    void handle_rmd(const std::string& params, ACE_SOCK_Stream& clientStream_);

    /**
     * @brief 处理 DELE 命令，删除服务器端的文件。
     *
     * @param params FTP 命令的参数，指定要删除的文件路径。
     * @param clientStream_ 与客户端通信的流。
     */
    void handle_dele(const std::string& params, ACE_SOCK_Stream& clientStream_);

    /**
     * @brief 处理 SIZE 命令，获取服务器端文件的大小。
     *
     * @param params FTP 命令的参数，指定要查询的文件路径。
     * @param clientStream_ 与客户端通信的流。
     */
    void handle_size(const std::string& params, ACE_SOCK_Stream& clientStream_);

    /**
     * @brief 处理 EPSV 命令，进入扩展被动模式。
     *
     * EPSV 模式允许客户端在控制连接上发出请求，并通过数据连接接收文件。
     *
     * @param clientStream_ 与客户端通信的流。
     */
    void handle_epsv(ACE_SOCK_Stream& clientStream_);

    /**
     * @brief 清除被动模式状态。
     *
     * 该方法关闭数据流和被动模式监听器，并重置被动模式状态。
     */
    void clear_passive_mode();

    /**
     * @brief 检查指定文件是否存在。
     *
     * @param fileName 要检查的文件路径。
     * @return 如果文件存在返回 true，否则返回 false。
     */
    bool file_exists(const std::string& fileName);

    // 数据连接相关
    ACE_SOCK_Acceptor dataAcceptor_; ///< 用于被动连接的监听器
    ACE_SOCK_Stream dataStream_;     ///< 客户端的数据连接流
    bool passive_mode_ = false;      ///< 标记是否启用了被动模式
};

#endif // FILECOMMAND_H
