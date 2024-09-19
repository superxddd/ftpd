#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

/**
 * @class ThreadPool
 * @brief 一个简单的线程池类，用于管理并发任务的执行。
 *
 * 该线程池类通过创建多个工作线程来并发执行任务。任务被添加到任务队列中，
 * 空闲的工作线程将从队列中提取任务并执行。线程池支持设置任务队列的最大长度，
 * 当队列满时，新任务将被拒绝。
 */
class ThreadPool
{
public:
    /**
     * @brief 构造函数，初始化线程池。
     *
     * 构造函数不会自动启动线程，需调用 `open` 方法来启动指定数量的线程。
     */
    ThreadPool();

    /**
     * @brief 析构函数，销毁线程池并确保所有线程结束。
     *
     * 调用 `close` 方法以确保线程池在销毁前正确关闭。
     */
    ~ThreadPool();

    /**
     * @brief 打开线程池并启动指定数量的线程。
     *
     * @param num_threads 要启动的线程数量，默认为 4。
     */
    void open(int num_threads = 4);

    /**
     * @brief 关闭线程池并等待所有线程结束。
     *
     * 调用此方法后，线程池将停止接收新任务，并在所有现有任务完成后关闭所有线程。
     */
    void close();

    /**
     * @brief 提交一个新任务到线程池。
     *
     * 该方法将一个任务加入到任务队列中。如果队列已满，任务将被拒绝。
     *
     * @tparam F 任务类型，可以是任何可调用对象（如函数、lambda 表达式等）。
     * @param task 要执行的任务。
     * @return 如果任务成功加入队列返回 true，否则返回 false。
     */
    template<class F>
    bool enqueue(F&& task)
    {
        {
            std::lock_guard<std::mutex> lock(queueMutex_);

            // 检查队列长度，超过最大值则拒绝任务并返回 false
            if (tasks_.size() >= max_queue_size_) {
                std::cerr << "Task queue is full. Rejecting new task."
                          << std::endl;
                return false; // 任务被拒绝
            }

            tasks_.emplace(std::forward<F>(task)); // 将任务加入队列
        }
        condition_.notify_one(); // 通知一个工作线程
        return true;             // 任务已成功加入队列
    }

    /**
     * @brief 设置任务队列的最大大小。
     *
     * @param max_size 任务队列的最大长度。
     */
    void set_max_queue_size(size_t max_size);

private:
    std::vector<std::thread> workers_;         ///< 工作线程列表
    std::queue<std::function<void()> > tasks_; ///< 任务队列
    std::mutex queueMutex_;             ///< 保护任务队列的互斥锁
    std::condition_variable condition_; ///< 条件变量，用于通知工作线程
    bool stop_ = false;           ///< 用于指示线程池是否应停止
    size_t max_queue_size_ = 100; ///< 任务队列的最大长度，默认 100
};

#endif // THREADPOOL_H
