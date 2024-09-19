#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <ace/SOCK_Stream.h>
#include <pwd.h>    // For getpwuid
#include <unistd.h> // For getuid

/**
 * @enum TransferMode
 * @brief 定义文件传输模式。
 *
 * 枚举类型 `TransferMode` 定义了两种文件传输模式：ASCII 和 BINARY。
 */
enum TransferMode
{
    ASCII, ///< ASCII 传输模式
    BINARY ///< 二进制传输模式
};

/**
 * @class Session
 * @brief 维护客户端会话状态的类。
 *
 * `Session` 类用于管理 FTP
 * 客户端的会话状态，包括登录状态、被动模式状态、传输模式、工作目录等信息。
 * 该类还提供与客户端通信的流。
 */
class Session
{
public:
    /**
     * @brief 构造函数，初始化会话。
     *
     * @param stream 与客户端通信的套接字流。
     */
    Session(ACE_SOCK_Stream& stream);

    /**
     * @brief 检查用户是否已登录。
     *
     * @return 如果用户已登录，返回 true；否则返回 false。
     */
    bool is_logged_in() const;

    /**
     * @brief 设置用户的登录状态。
     *
     * @param logged_in 指示用户是否已登录的布尔值。
     */
    void set_logged_in(bool logged_in);

    /**
     * @brief 检查会话是否处于被动模式。
     *
     * @return 如果处于被动模式，返回 true；否则返回 false。
     */
    bool is_passive_mode() const;

    /**
     * @brief 设置被动模式的状态。
     *
     * @param passive 指示是否启用被动模式的布尔值。
     */
    void set_passive_mode(bool passive);

    /**
     * @brief 获取当前的文件传输模式。
     *
     * @return 当前的传输模式（ASCII 或 BINARY）。
     */
    TransferMode get_transfer_mode() const;

    /**
     * @brief 设置文件传输模式。
     *
     * @param mode 要设置的传输模式（ASCII 或 BINARY）。
     */
    void set_transfer_mode(TransferMode mode);

    /**
     * @brief 获取当前的工作目录。
     *
     * @return 当前工作目录的字符串引用。
     */
    const std::string& get_working_directory() const;

    /**
     * @brief 设置当前的工作目录。
     *
     * @param dir 要设置的工作目录路径。
     */
    void set_working_directory(const std::string& dir);

    /**
     * @brief 获取当前会话的用户名。
     *
     * @return 当前会话的用户名。
     */
    const std::string& get_username() const;

    /**
     * @brief 设置用户名。
     *
     * @param username 要设置的用户名。
     */
    void set_username(const std::string& username);

    /**
     * @brief 获取与客户端通信的套接字流。
     *
     * @return 客户端通信的 ACE_SOCK_Stream 引用。
     */
    ACE_SOCK_Stream& get_client_stream();

    /**
     * @brief 获取当前用户的主目录。
     *
     * 该方法返回当前用户的主目录路径。
     *
     * @return 用户主目录的路径。
     */
    std::string get_home_directory();

private:
    ACE_SOCK_Stream& clientStream_; ///< 与客户端通信的套接字流
    bool logged_in_;                ///< 指示用户是否已登录
    bool passive_mode_;             ///< 指示是否处于被动模式
    TransferMode transfer_mode_;    ///< 当前的文件传输模式
    std::string working_directory_; ///< 当前的工作目录
    std::string username_;          ///< 当前会话的用户名
};

#endif // SESSION_H
