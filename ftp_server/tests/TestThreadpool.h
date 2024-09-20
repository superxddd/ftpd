#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>

class TestThreadpool {
public:
    TestThreadpool(size_t num_threads);
    ~TestThreadpool();

    // 提交任务到线程池
    void enqueue(std::function<void()> task);

private:
    // 线程执行函数
    void worker_thread();

    std::vector<std::thread> workers;                 // 线程池
    std::queue<std::function<void()>> tasks;          // 任务队列
    std::mutex queue_mutex;                           // 保护任务队列的互斥量
    std::condition_variable condition;                // 用于通知线程有新任务
    bool stop;                                        // 用于控制线程池停止
};

TestThreadpool::TestThreadpool(size_t num_threads) : stop(false) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back([this] { this->worker_thread(); });
    }
}

TestThreadpool::~TestThreadpool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();  // 通知所有线程停止
    for (std::thread& worker : workers) {
        worker.join();  // 等待所有线程完成
    }
}

void TestThreadpool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.push(std::move(task));
    }
    condition.notify_one();  // 通知一个线程处理任务
}

void TestThreadpool::worker_thread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] {
                return this->stop || !this->tasks.empty();
            });

            if (this->stop && this->tasks.empty()) {
                return;
            }

            task = std::move(this->tasks.front());
            this->tasks.pop();
        }
        task();  // 执行任务
    }
}
