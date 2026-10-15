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
    bool push(T value) {
        {
            std::lock_guard lock(m_mutex);
            if (m_closed) return false;

            m_queue.push(std::move(value));
        }
        m_cv.notify_one();
        return true;
    }

    // Blocks until an item is available or the queue is closed.
    // Returns nullopt when closed and empty.
    std::optional<T> pop() {
        std::unique_lock lock(m_mutex);

        m_cv.wait(lock, [this] {
            return m_closed || !m_queue.empty();
        });

        if (m_queue.empty()) return std::nullopt;

        T value = std::move(m_queue.front());
        m_queue.pop();
        return value;
    }

    void close() {
        {
            std::lock_guard lock(m_mutex);
            m_closed = true;
        }
        m_cv.notify_all();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<T> m_queue;
    bool m_closed = false;
};

#endif //THREADSAFEQUEUE_H
