#include "FileCommand.h"
#include <ace/Log_Msg.h>
#include <fstream>
#include <sstream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <random>
#include <list>
#include <sys/stat.h>
#include <ace/Message_Block.h>
#include <dirent.h>

// 定义每个传输块的大小
const size_t CHUNK_SIZE = 65536;          // 64KB
const size_t SMALL_FILE_THRESHOLD = 8192; // 小文件的阈值，8KB

// 构造函数
FileCommand::FileCommand(): dataAcceptor_(), dataStream_() {}

// 检查文件是否存在
bool FileCommand::file_exists(const std::string& fileName)
{
    struct stat buffer;
    return (stat(fileName.c_str(), &buffer) == 0);
}

void FileCommand::execute(
        Session& session,
        const std::string& name,
        const std::string& params,
        ACE_SOCK_Stream& clientStream_,
        ThreadPool& threadPool)
{
    if (!require_login(session)) {
        return;
    }
    if (name == "PASV") {
        handle_pasv(session, clientStream_);
    } else if (name == "TYPE") {
        handle_type(session, params, clientStream_);
    } else if (name == "STOR") {
        handle_stor(session, params, clientStream_, threadPool);
    } else if (name == "RETR") {
        handle_retr(session, params, clientStream_, threadPool);
    } else if (name == "LIST") {
        handle_list(session, clientStream_, threadPool);
    } else if (name == "MKD") {
        handle_mkd(session, params, clientStream_);
    } else if (name == "RMD") {
        handle_rmd(params, clientStream_);
    } else if (name == "DELE") {
        handle_dele(params, clientStream_);
    } else if (name == "SIZE") {
        handle_size(params, clientStream_);
    } else if (name == "EPSV") {
        handle_epsv(clientStream_);
    }
}

void FileCommand::handle_pasv(Session& session, ACE_SOCK_Stream& clientStream_)
{
    constexpr int kmax_retries_size = 5; // 最大重试次数
    std::random_device dev;
    std::mt19937 gen(dev());
    std::uniform_int_distribution<> distrib(
            1024, 65535); // 随机生成 1024 到 65535 的端口号

    ACE_INET_Addr serverAddr(
            (u_short)0, "127.0.0.1"); // 使用本地地址，端口为 0 表示使用随机端口

    int retries = 0;
    for (; retries < kmax_retries_size; ++retries) {
        // 设置随机端口
        serverAddr.set_port_number(distrib(gen));
        char addr_str[128] = {0};
        serverAddr.addr_to_string(addr_str, sizeof(addr_str));
        // ACE_DEBUG(
        //         (LM_DEBUG, "Trying to open passive mode on [%s]\n",
        //         addr_str));

        // 绑定到随机端口并监听
        if (dataAcceptor_.open(serverAddr) != -1) {
            break; // 成功启动
        }

        ACE_DEBUG(
                (LM_DEBUG, "Failed to open passive mode socket on [%s]\n",
                 addr_str));
    }

    if (retries >= kmax_retries_size) {
        ACE_ERROR((
                LM_ERROR,
                ACE_TEXT(
                        "(%P|%t) All retries failed to open passive mode socket\n")));
        std::string response = "500 Failed to enter passive mode.\r\n";
        clientStream_.send(response.c_str(), response.size());
        return;
    }

    // 获取服务器的本地地址和已分配的随机端口号
    dataAcceptor_.get_local_addr(serverAddr);

    // 获取服务器的 IP 地址和端口号
    std::string ipAddress = serverAddr.get_host_addr();
    unsigned short port = serverAddr.get_port_number();

    // 将 IP 地址转换为 FTP 协议需要的形式 (h1,h2,h3,h4)
    std::stringstream formattedIp;
    for (size_t i = 0; i < ipAddress.size(); ++i) {
        if (ipAddress[i] == '.') {
            formattedIp << ',';
        } else {
            formattedIp << ipAddress[i];
        }
    }

    // 将端口号分为高字节和低字节
    int p1 = port / 256;
    int p2 = port % 256;

    // 构建并发送 PASV 响应
    std::ostringstream response;
    response << "227 Entering Passive Mode (" << formattedIp.str() << "," << p1
             << "," << p2 << ").\r\n";
    clientStream_.send(response.str().c_str(), response.str().size());

    // ACE_DEBUG(
    //         (LM_DEBUG, "Server in passive mode on IP %s and port %d\n",
    //          serverAddr.get_host_addr(), port));

    // 标记进入被动模式
    passive_mode_ = true;
    session.set_passive_mode(true);
}

void FileCommand::handle_type(
        Session& session,
        const std::string& params,
        ACE_SOCK_Stream& clientStream_)
{
    std::string response;

    // 检查输入参数并设置传输模式
    if (params == "I") {
        session.set_transfer_mode(
                BINARY); // 假设 transfer_mode_
                         // 是一个成员变量，用于存储当前传输模式
        response = "200 Type set to I.\r\n"; // Binary mode
    } else if (params == "A") {
        session.set_transfer_mode(
                ASCII); // 假设 transfer_mode_
                        // 是一个成员变量，用于存储当前传输模式
        response = "200 Type set to A.\r\n"; // ASCII mode
    } else {
        response =
                "500 Unrecognized TYPE command. Supported types are I (binary) and A (ASCII).\r\n";
    }

    // 发送响应
    ssize_t bytesSent = clientStream_.send(response.c_str(), response.size());
    if (bytesSent == -1) {
        ACE_ERROR(
                (LM_ERROR,
                 ACE_TEXT("(%P|%t) Failed to send TYPE response to client\n")));
    } else {
        // ACE_DEBUG(
        //         (LM_DEBUG,
        //          ACE_TEXT("(%P|%t) Sent %zd bytes TYPE response to
        //          client\n"), bytesSent));
    }
}

//STOR命令
void FileCommand::handle_stor(
        Session& session,
        const std::string& params,
        ACE_SOCK_Stream& clientStream_,
        ThreadPool& threadPool)
{
    if (!passive_mode_) {
        std::string response = "425 Use PASV first.\r\n";
        clientStream_.send(response.c_str(), response.size());
        return;
    }

    // 使用线程池处理文件存储任务
    threadPool.enqueue([this, &session, params, &clientStream_] {
        // 等待客户端连接到被动模式的数据端口
        if (dataAcceptor_.accept(dataStream_) == -1) {
            std::string response = "425 Could not open data connection.\r\n";
            clientStream_.send(response.c_str(), response.size());
            return;
        }

        // 发送 150 响应，通知客户端数据连接已准备好
        std::string response150 = "150 Opening data connection.\r\n";
        clientStream_.send(response150.c_str(), response150.size());

        std::string fileName = params;

        // 接收数据并将其存储到临时缓冲区
        const size_t bufferSize = 65536; // 64KB buffer
        char buffer[bufferSize];
        ssize_t bytesReceived = 0;
        size_t totalBytesReceived = 0;

        std::vector<char> fileData;

        // 循环接收数据，直到客户端关闭连接
        while ((bytesReceived = dataStream_.recv(buffer, bufferSize)) > 0) {
            totalBytesReceived += bytesReceived;
            fileData.insert(fileData.end(), buffer, buffer + bytesReceived);
        }

        // 如果数据量小于 SMALL_FILE_THRESHOLD，则直接写入文件
        if (totalBytesReceived <= SMALL_FILE_THRESHOLD) {
            // 打开文件以进行写入
            std::ofstream outputFile(fileName, std::ios::binary);
            if (!outputFile.is_open()) {
                std::string response = "550 Failed to open file for writing.\r\n";
                clientStream_.send(response.c_str(), response.size());
                return;
            }

            // 直接将数据写入文件
            outputFile.write(fileData.data(), totalBytesReceived);
            if (outputFile.fail()) {
                std::string response = "550 Failed to write to file.\r\n";
                clientStream_.send(response.c_str(), response.size());
                outputFile.close();
                return;
            }

            outputFile.close();
        } else {
            // 如果是大文件，则使用 mmap 方式进行写入
            int fd = open(fileName.c_str(), O_RDWR | O_CREAT, 0644);
            if (fd == -1) {
                std::string response = "550 Failed to open file for writing.\r\n";
                clientStream_.send(response.c_str(), response.size());
                return;
            }

            // 将文件扩展到总接收数据大小
            if (ftruncate(fd, totalBytesReceived) == -1) {
                std::string response = "550 Failed to resize file.\r\n";
                clientStream_.send(response.c_str(), response.size());
                close(fd);
                return;
            }

            // 使用 mmap 将文件映射到内存
            void* fileMappedData = mmap(NULL, totalBytesReceived, PROT_WRITE, MAP_SHARED, fd, 0);
            if (fileMappedData == MAP_FAILED) {
                std::string response = "550 Failed to mmap file.\r\n";
                clientStream_.send(response.c_str(), response.size());
                close(fd);
                return;
            }

            // 将数据拷贝到 mmap 映射的内存中
            memcpy(fileMappedData, fileData.data(), totalBytesReceived);

            // 解除 mmap 映射
            munmap(fileMappedData, totalBytesReceived);

            // 关闭文件
            close(fd);
        }

        // 发送传输完成的响应
        std::string response = "226 Transfer complete.\r\n";
        clientStream_.send(response.c_str(), response.size());

        // 清理被动模式资源
        clear_passive_mode();
    });
}


//处理RETR
void FileCommand::handle_retr(
        Session& session,
        const std::string& params,
        ACE_SOCK_Stream& clientStream_,
        ThreadPool& threadPool)
{
    if (!passive_mode_) {
        std::string response = "425 Use PASV first.\r\n";
        clientStream_.send(response.c_str(), response.size());
        return;
    }

    // 使用线程池处理文件检索任务
    threadPool.enqueue([this, &session, params, &clientStream_] {
        // 等待客户端连接到被动模式的数据端口
        if (dataAcceptor_.accept(dataStream_) == -1) {
            std::string response = "425 Could not open data connection.\r\n";
            clientStream_.send(response.c_str(), response.size());
            return;
        }

        std::string fileName = params;
        if (!file_exists(fileName)) {
            std::string response = "550 File not found.\r\n";
            clientStream_.send(response.c_str(), response.size());
            return;
        }

        // 打开文件并获取文件大小
        int fd = open(fileName.c_str(), O_RDONLY);
        if (fd == -1) {
            std::string response = "550 Failed to open file.\r\n";
            clientStream_.send(response.c_str(), response.size());
            return;
        }

        struct stat fileStat;
        if (fstat(fd, &fileStat) == -1) {
            std::string response = "550 Failed to get file size.\r\n";
            clientStream_.send(response.c_str(), response.size());
            close(fd);
            return;
        }

        size_t fileSize = fileStat.st_size;

        // 发送 150 响应，通知客户端即将开始文件传输
        std::string response150 = "150 Opening data connection.\r\n";
        clientStream_.send(response150.c_str(), response150.size());

        ssize_t bytesSent = 0;

        if (fileSize <= SMALL_FILE_THRESHOLD) {
            // 对于小文件，直接读取整个文件到内存并一次性发送
            char* fileData = new char[fileSize];
            if (read(fd, fileData, fileSize) !=
                static_cast<ssize_t>(fileSize)) {
                std::string response = "550 Failed to read file.\r\n";
                clientStream_.send(response.c_str(), response.size());
                delete[] fileData;
                close(fd);
                return;
            }

            bytesSent = dataStream_.send_n(fileData, fileSize);
            delete[] fileData;
            if (bytesSent == -1) {
                std::string response =
                        "426 Transfer aborted: Connection closed.\r\n";
                clientStream_.send(response.c_str(), response.size());
                close(fd);
                return;
            }
        } else {
            // 对于大文件，使用 mmap 和分块传输
            void* fileData =
                    mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
            if (fileData == MAP_FAILED) {
                std::string response = "550 Failed to mmap file.\r\n";
                clientStream_.send(response.c_str(), response.size());
                close(fd);
                return;
            }

            size_t offset = 0;
            while (offset < fileSize) {
                size_t bytesToSend = std::min(CHUNK_SIZE, fileSize - offset);

                // 从内存中发送当前块
                bytesSent = dataStream_.send_n(
                        (char*)fileData + offset, bytesToSend);
                if (bytesSent == -1) {
                    std::string response =
                            "426 Transfer aborted: Connection closed.\r\n";
                    clientStream_.send(response.c_str(), response.size());
                    munmap(fileData, fileSize);
                    close(fd);
                    return;
                }

                offset += bytesToSend;
            }

            // 释放 mmap 的内存映射
            munmap(fileData, fileSize);
        }

        // 检查传输是否成功完成
        if (bytesSent != -1) {
            std::string response = "226 Transfer complete.\r\n";
            clientStream_.send(response.c_str(), response.size());
        } else {
            std::string response = "426 Transfer aborted due to error.\r\n";
            clientStream_.send(response.c_str(), response.size());
        }

        // 关闭文件描述符
        close(fd);

        // 关闭数据连接
        dataStream_.close();
        clear_passive_mode();
    });
}

// 处理 LIST 命令
void FileCommand::handle_list(
        Session& session,
        ACE_SOCK_Stream& clientStream_,
        ThreadPool& threadPool)
{
    if (!passive_mode_) {
        std::string response = "425 Use PASV first.\r\n";
        clientStream_.send(response.c_str(), response.size());
        return;
    }

    // 使用线程池处理 LIST 命令
    // threadPool.enqueue([this, &session, &clientStream_] {
    // 等待客户端连接到被动模式的数据端口
    dataAcceptor_.accept(dataStream_);

    // 发送 150 响应，通知客户端即将开始传输目录列表
    std::string response150 = "150 Here comes the directory listing.\r\n";
    clientStream_.send(response150.c_str(), response150.size());

    // 获取当前工作目录
    std::string currentDir = session.get_working_directory();
    std::ostringstream dirList;

    // 使用系统调用列出目录内容
    FILE* pipe = popen(("LANG=en_US.UTF-8 ls -ln " + currentDir).c_str(), "r");
    if (!pipe) {
        std::string response = "550 Could not open directory.\r\n";
        clientStream_.send(response.c_str(), response.size());
        return;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        dirList << buffer;
    }
    pclose(pipe);

    // 发送目录列表
    std::string listResponse = dirList.str();
    dataStream_.send(listResponse.c_str(), listResponse.size());

    // 发送完成响应 226
    std::string response226 = "226 Directory send OK.\r\n";
    clientStream_.send(response226.c_str(), response226.size());

    // 清除被动模式状态
    clear_passive_mode();
    // });
}

// 处理 MKD 命令
void FileCommand::handle_mkd(
        Session& session,
        const std::string& params,
        ACE_SOCK_Stream& clientStream_)
{
    // 检查参数是否为空
    if (params.empty()) {
        std::string response = "550 Directory name not specified.\r\n";
        clientStream_.send(response.c_str(), response.size());
        return;
    }

    // 获取当前工作目录
    std::string currentDir =
            session.get_working_directory(); // 确保 Session 类有这个方法
    std::string newDirPath =
            currentDir + "/" + params; // 组合当前目录和新目录名称

    // 检查目录是否已经存在
    if (file_exists(newDirPath)) {
        std::string response = "550 Directory already exists.\r\n";
        clientStream_.send(response.c_str(), response.size());
        return;
    }

    // 创建目录
    if (mkdir(newDirPath.c_str(), 0755) == 0) {
        std::string response = "257 Directory created.\r\n";
        clientStream_.send(response.c_str(), response.size());
    } else {
        // 获取错误信息
        std::string errorMessage = strerror(errno);
        std::string response =
                "550 Failed to create directory: " + errorMessage + "\r\n";
        clientStream_.send(response.c_str(), response.size());
    }
}

// 处理 RMD 命令
void FileCommand::handle_rmd(
        const std::string& params,
        ACE_SOCK_Stream& clientStream_)
{
    if (rmdir(params.c_str()) == 0) {
        std::string response = "250 Directory deleted.\r\n";
        clientStream_.send(response.c_str(), response.size());
    } else {
        std::string response = "550 Failed to remove directory.\r\n";
        clientStream_.send(response.c_str(), response.size());
    }
}

// 处理 DELE 命令
void FileCommand::handle_dele(
        const std::string& params,
        ACE_SOCK_Stream& clientStream_)
{
    if (remove(params.c_str()) == 0) {
        std::string response = "250 File deleted.\r\n";
        clientStream_.send(response.c_str(), response.size());
    } else {
        std::string response = "550 Failed to delete file.\r\n";
        clientStream_.send(response.c_str(), response.size());
    }
}

// 处理 SIZE 命令
void FileCommand::handle_size(
        const std::string& params,
        ACE_SOCK_Stream& clientStream_)
{
    struct stat buffer;
    if (stat(params.c_str(), &buffer) == 0) {
        std::ostringstream response;
        response << "213 " << buffer.st_size << "\r\n"; // 返回文件大小
        clientStream_.send(response.str().c_str(), response.str().size());
    } else {
        std::string response = "550 File not found.\r\n";
        clientStream_.send(response.c_str(), response.size());
    }
}

// 处理 EPSV 命令
void FileCommand::handle_epsv(ACE_SOCK_Stream& clientStream_)
{
    // 绑定到一个随机端口并监听
    ACE_INET_Addr serverAddr(
            (u_short)0, "127.0.0.1"); // 使用 127.0.0.1 作为本地地址，端口为 0
                                      // 表示使用随机端口
    if (dataAcceptor_.open(serverAddr) == -1) {
        ACE_ERROR(
                (LM_ERROR,
                 ACE_TEXT("(%P|%t) Failed to open EPSV mode socket\n")));
        std::string response = "500 Failed to enter extended passive mode.\r\n";
        clientStream_.send(response.c_str(), response.size());
        return;
    }

    // 获取服务器的本地地址和已分配的随机端口号
    dataAcceptor_.get_local_addr(serverAddr);

    // 获取分配的端口号
    unsigned short port = serverAddr.get_port_number();

    // 构建并发送 EPSV 响应，格式为 (|||port|)
    std::ostringstream response;
    response << "229 Entering Extended Passive Mode (|||" << port << "|).\r\n";
    clientStream_.send(response.str().c_str(), response.str().size());

    ACE_DEBUG((LM_DEBUG, "Server in extended passive mode on port %d\n", port));

    passive_mode_ = true; // 标记被动模式
}

// 完成后清理被动模式的资源
void FileCommand::clear_passive_mode()
{
    if (dataStream_.get_handle() != ACE_INVALID_HANDLE) {
        dataStream_.close(); // 关闭数据流
    }
    if (dataAcceptor_.get_handle() != ACE_INVALID_HANDLE) {
        dataAcceptor_.close(); // 关闭监听的被动端口
    }
    passive_mode_ = false; // 清除被动模式标志
}
