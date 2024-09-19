#include "CwdCommand.h"
#include <iostream>
#include <ace/OS.h> // For ACE_OS::chdir, ACE_OS::getcwd, ACE_OS::realpath
#include <cstring>  // For string operations
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>

/**
 * @brief 执行 CWD 命令。
 *
 * 该方法实现了更改当前会话工作目录的逻辑。如果提供的路径无效，向客户端返回错误消息。
 * 如果路径有效，会将工作目录更改为新目录，并发送确认响应给客户端。
 */
void CwdCommand::execute(
        Session& session,
        const std::string& /*name*/,
        const std::string& params,
        ACE_SOCK_Stream& clientStream_,
        ThreadPool& /*threadPool*/)
{
    // 检查用户是否已经登录
    if (!require_login(session)) {
        return;
    }

    std::string newPath = params;

    // 检查路径是否为空
    if (newPath.empty()) {
        std::string response =
                "550 Failed to change directory. Path not specified.\r\n";
        clientStream_.send(response.c_str(), response.size());
        return;
    }

    // 获取当前工作目录
    char currentDir[1024];
    if (ACE_OS::getcwd(currentDir, sizeof(currentDir)) == nullptr) {
        std::string response =
                "550 Failed to get current working directory.\r\n";
        clientStream_.send(response.c_str(), response.size());
        return;
    }

    std::string fullPath;

    // 处理相对路径和绝对路径
    if (newPath == "..") {
        // 处理上层目录
        fullPath = std::string(currentDir) + "/..";
    } else if (newPath[0] != '/') {
        // 相对路径
        fullPath = std::string(currentDir) + "/" + newPath;
    } else {
        // 绝对路径
        fullPath = newPath;
    }

    // 使用 realpath 来解析实际路径
    char resolvedPath[1024];
    if (ACE_OS::realpath(fullPath.c_str(), resolvedPath) == nullptr) {
        std::string response =
                "550 Failed to resolve path: \"" + fullPath + "\".\r\n";
        clientStream_.send(response.c_str(), response.size());
        return;
    }

    // 检查路径是否存在并且是目录
    struct stat info;
    if (stat(resolvedPath, &info) != 0 || !(info.st_mode & S_IFDIR)) {
        std::string response =
                "550 Directory does not exist or is not a directory: \"" +
                std::string(resolvedPath) + "\".\r\n";
        clientStream_.send(response.c_str(), response.size());
        return;
    }

    // 尝试改变工作目录
    if (ACE_OS::chdir(resolvedPath) == -1) {
        // 失败时返回550错误
        std::string response = "550 Failed to change directory to \"" +
                               std::string(resolvedPath) + "\".\r\n";
        clientStream_.send(response.c_str(), response.size());
    } else {
        // 成功时返回250响应
        std::string response = "250 Directory successfully changed to \"" +
                               std::string(resolvedPath) + "\".\r\n";
        session.set_working_directory(resolvedPath);
        clientStream_.send(response.c_str(), response.size());
    }
}

// // expand_tilde 函数的实现
// std::string CwdCommand::expand_tilde(const std::string& path) {
//     if (path.empty()) {
//         return path;
//     }

//     std::string expandedPath = path;
//     if (path[0] == '~') {
//         std::string homeDir;
//         struct passwd* pw = nullptr;

//         // 如果路径为 "~" 或 "~/path"，展开为当前用户的主目录
//         if (path.size() == 1 || path[1] == '/') {
//             pw = getpwuid(getuid());
//             homeDir = pw ? pw->pw_dir : "/";
//         } else {
//             // "~username/path"，展开为指定用户的主目录
//             size_t pos = path.find('/');
//             std::string username = (pos == std::string::npos) ?
//             path.substr(1) : path.substr(1, pos - 1); pw =
//             getpwnam(username.c_str()); homeDir = pw ? pw->pw_dir : "/";
//         }

//         if (!homeDir.empty()) {
//             expandedPath = homeDir + path.substr(path.find_first_of('/'));
//         } else {
//             // 如果没有找到主目录，返回一个错误或空字符串
//             expandedPath = "";  // 如果没有找到主目录，设置为空
//         }
//     }
//     return expandedPath;
// }
