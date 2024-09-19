#ifndef WORKER_REACTOR_TASK_H
#define WORKER_REACTOR_TASK_H

#include <ace/Task.h>
#include <ace/Reactor.h>

/**
 * @class WorkerReactorTask
 * @brief 负责管理 ACE Reactor 的工作线程类。
 *
 * 该类继承自 `ACE_Task_Base`，用于在单独的线程中运行
 * `ACE_Reactor`，从而实现事件驱动的任务处理。 `WorkerReactorTask`
 * 启动一个工作线程并在其中运行 `ACE_Reactor` 的事件循环，直到调用 `stop()`
 * 方法停止事件循环。
 */
class WorkerReactorTask: public ACE_Task_Base
{
public:
    /**
     * @brief 构造函数，初始化 WorkerReactorTask 对象。
     *
     * 构造函数初始化 `reactor_` 指针为 nullptr，具体的 Reactor 实例将在 `svc()`
     * 方法中创建。
     */
    WorkerReactorTask();

    /**
     * @brief 启动工作线程并运行 Reactor 的事件循环。
     *
     * @return 如果成功启动线程，返回 0；否则返回 -1。
     */
    int start();

    /**
     * @brief 停止 Reactor 的事件循环并关闭线程。
     */
    void stop();

    /**
     * @brief 获取当前线程中的 `ACE_Reactor` 实例。
     *
     * @return 指向当前线程的 `ACE_Reactor` 实例的指针。
     */
    ACE_Reactor* get_reactor();

protected:
    /**
     * @brief 线程服务函数，在独立的线程中运行 `ACE_Reactor` 的事件循环。
     *
     * 该方法在调用 `start()` 时启动，它创建 `ACE_Reactor` 实例并运行事件循环，
     * 直到调用 `stop()` 停止事件循环。
     *
     * @return 线程退出时返回 0。
     */
    virtual int svc() override;

private:
    ACE_Reactor* reactor_; ///< 指向当前工作线程中 `ACE_Reactor` 实例的指针
};

#endif // WORKER_REACTOR_TASK_H
