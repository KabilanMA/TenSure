#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

using namespace std;

using Task = std::function<void()>;

class ThreadPool {
private:
    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex queue_mutex;
    condition_variable condition;
    atomic<bool> stop;

    void worker_loop();

public:
    ThreadPool(size_t threads);

    // Function to add work to the queue
    void enqueue(Task task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks.push(std::move(task));
        }
        // Notify one waiting worker thread that new work is available
        condition.notify_one();
    }

    // Destructor: Stop all workers gracefully
    ~ThreadPool();
};