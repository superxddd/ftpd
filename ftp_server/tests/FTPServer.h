#include "MasterAcceptor.h"
#include "ThreadPool.h"
#include <ace/INET_Addr.h>
#include "WorkerReactorTask.h"
#include <ace/Reactor.h>
#include <ace/Log_Msg.h>
#include <thread>
#include <ace/TP_Reactor.h>

class FTPServer {
public:
    FTPServer(int port, int numWorkers, int numThreadPoolThreads) {
        // 初始化 WorkerReactorTask
        ACE_TP_Reactor* tp_reactor = new ACE_TP_Reactor();
        ACE_Reactor* reactor = new ACE_Reactor(tp_reactor); // 使用 TP_Reactor 构造 ACE_Reactor
        ACE_Reactor::instance(reactor);  // 将其设置为全局 Reactor

        workerTasks = new WorkerReactorTask*[numWorkers];
        threadPool = new ThreadPool();

        // 启动工作线程
    for (int i = 0; i < numWorkers; ++i) {
        workerTasks[i] = new WorkerReactorTask();
        if (workerTasks[i]->start() == -1) {
            ACE_ERROR((LM_ERROR, "Failed to start worker task.\n"));
        }
    }
        // 启动线程池
        threadPool->open(numThreadPoolThreads);

        // 初始化 MasterAcceptor 并监听端口
        acceptor = new MasterAcceptor(workerTasks, numWorkers, *threadPool);
        ACE_INET_Addr addr(port, "127.0.0.1");
        if (acceptor->open(addr) == -1) {
            throw std::runtime_error("Failed to start server.");
        }

        // 启动 Reactor 事件循环
    serverThread = std::thread([]() {
        ACE_Reactor::instance()->run_reactor_event_loop();
    });
    }

    ~FTPServer() {
        // 停止所有工作线程
        for (int i = 0; i < numWorkers; ++i) {
            workerTasks[i]->stop();
            delete workerTasks[i];
        }

        // 关闭线程池
        threadPool->close();
        delete[] workerTasks;
        delete threadPool;
        // 停止服务器线程
        ACE_Reactor::instance()->end_reactor_event_loop();
        serverThread.join();
        delete ACE_Reactor::instance();
    }

private:
    MasterAcceptor* acceptor;
    WorkerReactorTask** workerTasks;
    ThreadPool* threadPool;
    std::thread serverThread;
    int numWorkers;
};
