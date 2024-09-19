#ifndef MASTER_ACCEPTOR_H
#define MASTER_ACCEPTOR_H

#include <ace/Event_Handler.h>
#include <ace/SOCK_Acceptor.h>
#include "WorkerReactorTask.h"
#include "ThreadPool.h"

/**
 * @class MasterAcceptor
 * @brief 负责接收客户端连接并分发给工作线程的类。
 *
 * MasterAcceptor 类是 ACE
 * 框架中的事件处理器，负责监听传入的客户端连接请求，并将其分配给工作线程进行处理。
 * 它通过 ACE_SOCK_Acceptor 接受客户端的 TCP 连接，并通过 WorkerReactorTask
 * 处理并发连接。
 */
class MasterAcceptor: public ACE_Event_Handler
{
public:
    /**
     * @brief 构造函数，初始化 MasterAcceptor。
     *
     * @param worker_tasks 指向 WorkerReactorTask
     * 的指针数组，用于处理多个客户端连接。
     * @param num_workers 工作线程的数量。
     * @param threadPool 线程池的引用，用于管理并发任务。
     */
    MasterAcceptor(
            WorkerReactorTask **worker_tasks,
            int num_workers,
            ThreadPool &threadPool);

    /**
     * @brief 打开监听端口并开始接受客户端连接。
     *
     * @param listen_addr 要监听的 IP 地址和端口。
     * @return 如果成功则返回 0，失败则返回 -1。
     */
    int open(const ACE_INET_Addr &listen_addr);

    /**
     * @brief 获取用于事件处理的句柄。
     *
     * @return 监听套接字的句柄。
     */
    virtual ACE_HANDLE get_handle() const override;

    /**
     * @brief 处理输入事件（即新客户端连接）。
     *
     * 当有新的客户端连接到达时，MasterAcceptor
     * 会接受连接并将其分配给工作线程处理。
     *
     * @param fd 客户端的文件描述符（此处未使用）。
     * @return 如果成功则返回 0，失败则返回 -1。
     */
    virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE) override;

    /**
     * @brief 处理关闭事件。
     *
     * 当 MasterAcceptor 关闭时，关闭监听套接字并释放相关资源。
     *
     * @param handle 关闭的句柄。
     * @param close_mask 关闭掩码。
     * @return 总是返回 0。
     */
    virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask)
            override;

private:
    ACE_SOCK_Acceptor acceptor_; ///< 用于接受客户端连接的 ACE_SOCK_Acceptor
    WorkerReactorTask **worker_tasks_; ///< 工作线程的数组，用于处理客户端连接
    int num_workers_;                  ///< 工作线程的数量
    int next_worker_;        ///< 轮询选择的下一个工作线程的索引
    ThreadPool &threadPool_; ///< 线程池的引用，用于并发任务管理
};

#endif // MASTER_ACCEPTOR_H
