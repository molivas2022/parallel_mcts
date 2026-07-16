/*
 * Thread safe queue and concurrency utilities
 */

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <utility>

template <typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    
    // flag to release waiting threads when shutddown
    bool shutdown_flag_ = false;

public:
    ThreadSafeQueue() = default;
    
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // pushes a new item into the queue and wakes up exactly one sleeping worker thread
    void push(T value) {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(std::move(value));
        cv_.notify_one();
    }

    // used by workers: if the queue is empty, the thread goes to sleep
    // it wakes up when a new item is pushed or when the queue is shut down
    // returns false if the queue was shut down while empty
    bool wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(mtx_);
        
        // Predicate prevents spurious wakeups
        cv_.wait(lock, [this]() { 
            return !queue_.empty() || shutdown_flag_; 
        });
        
        if (shutdown_flag_ && queue_.empty()) {
            return false;
        }
        
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    // used by the master thread to instantly check for completed results without blocking the tree
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.empty()) {
            return false;
        }
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    // safely unblocks all waiting threads and flags on shutdown
    void shutdown() {
        std::lock_guard<std::mutex> lock(mtx_);
        shutdown_flag_ = true;
        cv_.notify_all();
    }
    
    bool is_empty() {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }
};