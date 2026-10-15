//
// Created by Birka on 15/10/2026.
//

#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>
#include <optional>
#include <utility>

template <typename T>
class ThreadSafeQueue {
public:
    void push(T value) {
        {
            std::lock_guard lock(mutex_);
            if (closed_) return;

            queue_.push(std::move(value));
        }
        cv_.notify_one();
    }

    // Blocks until an item is available or the queue is closed.
    // Returns nullopt when closed and empty.
    std::optional<T> pop() {
        std::unique_lock lock(mutex_);

        cv_.wait(lock, [this] {
            return closed_ || !queue_.empty();
        });

        if (queue_.empty()) return std::nullopt;

        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    void close() {
        {
            std::lock_guard lock(mutex_);
            closed_ = true;
        }
        cv_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
    bool closed_ = false;
};

#endif //THREADSAFEQUEUE_H
