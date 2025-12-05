#include "tensure/ThreadPool.hpp"

// The thread's main loop function
void ThreadPool::worker_loop() {
    for (;;) {
        Task task;
        {
            // A. Acquire lock to check queue
            std::unique_lock<std::mutex> lock(this->queue_mutex);

            // B. Wait until queue is NOT empty OR the pool is stopped
            this->condition.wait(lock,
                [this]{ return this->stop || !this->tasks.empty(); });

            // C. If stopped and queue is empty, exit thread loop
            if (this->stop && this->tasks.empty())
                return;

            // D. Take task from queue
            task = std::move(this->tasks.front());
            this->tasks.pop();
        } // Lock automatically released here

        // E. Execute task (outside the lock)
        task();
    }
}

// Constructor Implementation
ThreadPool::ThreadPool(size_t threads) : stop(false) {
    if (threads == 0) threads = 1; // Ensure at least one thread

    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back(&ThreadPool::worker_loop, this);
    }
}

// Destructor Implementation
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all(); // Wake up all waiting threads
    for(std::thread &worker: workers) {
        if(worker.joinable())
            worker.join(); // Wait for each thread to finish
    }
}