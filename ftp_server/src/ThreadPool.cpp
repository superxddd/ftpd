#include "ThreadPool.h"

ThreadPool::ThreadPool() = default;

ThreadPool::~ThreadPool()
{
    close();
}

void ThreadPool::open(int num_threads)
{
    stop_ = false; // 确保线程池在启动时可以正常工作
    for (int i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex_);
                    this->condition_.wait(lock, [this] {
                        return this->stop_ || !this->tasks_.empty();
                    });
                    if (this->stop_ && this->tasks_.empty())
                        return; // 线程退出
                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }
                task(); // 执行任务
            }
        });
    }
}

void ThreadPool::close()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        stop_ = true; // 设置线程池停止标志
    }
    condition_.notify_all(); // 唤醒所有等待中的线程

    for (std::thread &worker : workers_) {
        if (worker.joinable()) {
            worker.join(); // 等待线程完成任务
        }
    }
    workers_.clear(); // 清空线程池
}

void ThreadPool::set_max_queue_size(size_t max_size)
{
    max_queue_size_ = max_size;
}
