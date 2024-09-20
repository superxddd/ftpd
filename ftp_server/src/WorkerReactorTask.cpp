#include "WorkerReactorTask.h"
#include <ace/Log_Msg.h>

WorkerReactorTask::WorkerReactorTask(): reactor_(nullptr) {}

int WorkerReactorTask::start()
{
    return this->activate(THR_NEW_LWP | THR_JOINABLE, 1);
}

void WorkerReactorTask::stop()
{
    if (reactor_ != nullptr) {
        reactor_->end_reactor_event_loop(); // 停止事件循环
    }
    this->wait(); // 等待线程结束
    if (reactor_ != nullptr) {
        reactor_->close(); // 关闭 Reactor
        delete reactor_;   // 释放 Reactor 动态内存
        reactor_ = nullptr;
    }
}

ACE_Reactor* WorkerReactorTask::get_reactor()
{
    return reactor_;
}

int WorkerReactorTask::svc()
{
    // 动态分配 Reactor，确保其生命周期与 WorkerReactorTask 绑定
    reactor_ = new ACE_Reactor();

    // ACE_DEBUG(
    //         (LM_DEBUG,
    //          "(Reactor Thread ID: %t) Reactor event loop starting.\n"));

    // 启动事件循环
    reactor_->run_reactor_event_loop();

    // 事件循环结束后，打印日志
    ACE_DEBUG(
            (LM_DEBUG, "(Reactor Thread ID: %t) Reactor event loop ending.\n"));

    return 0;
}
