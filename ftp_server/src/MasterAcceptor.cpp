#include "MasterAcceptor.h"
#include "ClientHandler.h"
#include <ace/Log_Msg.h>
#include <map>

std::map<std::string, std::string> ps_map{};
MasterAcceptor::MasterAcceptor(
        WorkerReactorTask** worker_tasks,
        int num_workers,
        ThreadPool& threadPool)
    : worker_tasks_(worker_tasks),
      num_workers_(num_workers),
      next_worker_(0),
      threadPool_(threadPool)
{
    this->reactor(ACE_Reactor::instance()); // 使用 ACE Reactor
}

int MasterAcceptor::open(const ACE_INET_Addr& listen_addr)
{
    // 打开监听套接字
    if (this->acceptor_.open(listen_addr) == -1) {
        ACE_ERROR_RETURN((LM_ERROR, "Failed to open acceptor\n"), -1);
    }
    // 将当前对象注册到 Reactor 中，监听 ACCEPT 事件
    return this->reactor()->register_handler(
            this, ACE_Event_Handler::ACCEPT_MASK);
}

ACE_HANDLE MasterAcceptor::get_handle() const
{
    return this->acceptor_.get_handle(); // 返回 acceptor 的句柄
}

int MasterAcceptor::handle_input(ACE_HANDLE /*fd*/)
{
    ACE_SOCK_Stream clientStream;

    // 接受来自客户端的连接
    if (this->acceptor_.accept(clientStream) == -1) {
        ACE_ERROR_RETURN((LM_ERROR, "Failed to accept client\n"), -1);
    }

    // 选择下一个工作线程（循环选择）
    WorkerReactorTask* worker_task = worker_tasks_[next_worker_];
    next_worker_ = (next_worker_ + 1) % num_workers_; // 轮询选择工作线程

    // 创建一个新的 ClientHandler 处理客户端请求
    ClientHandler* handler = new ClientHandler(
            clientStream, worker_task->get_reactor(), threadPool_);

    // 打开 ClientHandler，如果失败则关闭连接
    if (handler->open() == -1) {
        handler->handle_close(ACE_INVALID_HANDLE, 0);
    }

    return 0;
}

int MasterAcceptor::handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask)
{
    this->acceptor_.close();
    return 0;
}
