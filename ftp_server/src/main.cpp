#include <csignal> // For signal handling
#include <ace/Reactor.h>
#include <ace/Log_Msg.h>
#include <map>
#include <sstream>
#include <fstream>
#include "WorkerReactorTask.h"
#include "ThreadPool.h"
#include "MasterAcceptor.h"
#include <iostream> // For std::stoi
#include <atomic>

// 全局变量声明
MasterAcceptor* acceptor = nullptr;         ///< 主接收器指针
WorkerReactorTask** worker_tasks = nullptr; ///< WorkerReactorTask 指针数组
ThreadPool* threadPool = nullptr;           ///< 线程池指针
int num_workers = 4;                        ///< 工作线程数量
std::atomic<bool> shutting_down(false); ///< 标志位，表示服务器是否正在关闭
extern std::map<std::string, std::string> ps_map; //用户信息表

/**
 * @brief 读取用户凭据文件并生成用户映射表。
 *
 * @param ps_map 用户名和密码的映射表。
 */
void make_map(std::map<std::string, std::string>& ps_map)
{
    std::string userFile_ = "userfile.txt"; // 这里你需要指定实际的文件名
    std::ifstream file(userFile_);
    if (!file.is_open()) {
        ACE_DEBUG((LM_ERROR, ACE_TEXT("Couldn't find userfile.txt\n")));
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string stored_username, stored_password;
        if (iss >> stored_username >> stored_password) {
            ps_map[stored_username] = stored_password;
        }
    }
}

/**
 * @brief 捕获 SIGINT 信号并关闭服务器。
 *
 * @param signal 捕获到的信号值。
 */
void handle_signal(int signal)
{
    if (signal == SIGINT && !shutting_down) {
        shutting_down = true;
        ACE_DEBUG((LM_DEBUG, "Received SIGINT, shutting down server...\n"));

        // 使用 ACE_Reactor::notify 触发 Reactor 的关闭，避免多线程问题
        ACE_Reactor::instance()->end_reactor_event_loop();
    }
}

int main(int argc, char* argv[])
{
    // 检查命令行参数
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <port> [num_workers] [num_threadpool_threads]\n";
        return 1;
    }

    // 尝试将命令行参数转换为端口号
    int port = 0;
    try {
        port = std::stoi(argv[1]); // 将命令行参数转为整数
    } catch (const std::exception& e) {
        std::cerr << "Invalid port number: " << argv[1] << std::endl;
        return 1;
    }

    // 设置 WorkerReactorTask 的线程数量，默认为 4
    int num_workers = 4;
    if (argc >= 3) {
        try {
            num_workers = std::stoi(argv[2]);
        } catch (const std::exception& e) {
            std::cerr << "Invalid number of reactor workers: " << argv[2]
                      << std::endl;
            return 1;
        }
    }

    // 设置 ThreadPool 的线程数量，默认为 4
    int num_threadpool_threads = 4;
    if (argc >= 4) {
        try {
            num_threadpool_threads = std::stoi(argv[3]);
        } catch (const std::exception& e) {
            std::cerr << "Invalid number of thread pool threads: " << argv[3]
                      << std::endl;
            return 1;
        }
    }

    // 捕获 SIGINT 信号 (Ctrl + C)
    std::signal(SIGINT, handle_signal);

    worker_tasks = new WorkerReactorTask*[num_workers];
    threadPool = new ThreadPool();

    make_map(ps_map);

    // 创建从 Reactor 任务并启动 Reactor 线程池
    for (int i = 0; i < num_workers; ++i) {
        worker_tasks[i] = new WorkerReactorTask();
        if (worker_tasks[i]->start() == -1) {
            ACE_ERROR_RETURN((LM_ERROR, "Failed to start worker task.\n"), 1);
        }
    }

    // 打开线程池
    threadPool->open(num_threadpool_threads);

    // 创建 MasterAcceptor 并启动服务器
    acceptor = new MasterAcceptor(worker_tasks, num_workers, *threadPool);
    ACE_INET_Addr addr(port, "127.0.0.1"); // 使用输入的端口号
    if (acceptor->open(addr) == -1) {
        ACE_ERROR_RETURN((LM_ERROR, "Failed to open server\n"), 1);
    }

    // 启动主 Reactor 的事件循环
    ACE_DEBUG((
            LM_DEBUG,
            "Server started on port %d, with %d reactor workers and %d thread pool threads.\n",
            port, num_workers, num_threadpool_threads));
    ACE_Reactor::instance()->run_reactor_event_loop();

    // 确保所有线程和资源在关闭时正确处理
    if (shutting_down) {
        // 停止所有 Worker Reactor 任务
        for (int i = 0; i < num_workers; ++i) {
            if (worker_tasks[i]) {
                worker_tasks[i]->stop();
            }
        }

        // 停止线程池
        threadPool->close();

        // 删除主接收器
        delete[] worker_tasks;
        delete threadPool;

        ACE_DEBUG((LM_DEBUG, "Server shutdown complete.\n"));
    }

    return 0;
}
