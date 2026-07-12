/*
 * Module: Thread-Safe Queue and Concurrency Utilities
 *
 * This file provides the synchronization primitives required for Pipeline 
 * (Master-Worker) Tree Parallelization. Unlike standard OpenMP loops which 
 * execute synchronously, the WU-UCT algorithm requires long-lived worker 
 * threads that idle asynchronously until work is available.
 *
 * The ThreadSafeQueue safely passes data (like pending simulations or 
 * completed results) between the master thread and worker threads without 
 * data races. It uses a condition variable to put worker threads to sleep 
 * when the queue is empty, ensuring they consume 0% CPU while waiting, 
 * and immediately wakes them when a task is pushed.
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
    
    // Flag to gracefully release waiting threads during application shutdown
    bool shutdown_flag_ = false;

public:
    ThreadSafeQueue() = default;
    
    // Disable copying to prevent accidental duplication of mutexes
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    /*
     * Pushes a new item into the queue and wakes up exactly one sleeping 
     * worker thread to process it.
     */
    void push(T value) {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(std::move(value));
        cv_.notify_one();
    }

    /*
     * Used by worker threads. If the queue is empty, the thread goes to sleep.
     * It wakes up when a new item is pushed or when the queue is shut down.
     * Returns false if the queue was shut down while empty.
     */
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

    /*
     * Used by the master thread to instantly check for completed results 
     * without blocking its MCTS tree traversal loop.
     */
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.empty()) {
            return false;
        }
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    /*
     * Safely unblocks all waiting threads and flags the queue as shutting down.
     * Required to prevent deadlocks when the MCTS agent is destroyed.
     */
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