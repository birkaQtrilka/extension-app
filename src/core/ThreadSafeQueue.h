//
// Created by Birka on 15/10/2026.
//

#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <condition_variable>
#include <expected>
#include <mutex>
#include <queue>
#include <optional>
#include <utility>

enum class PopFailure {
    Timeout,
    Closed
};

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
    std::expected<T, PopFailure> pop() {
        std::unique_lock lock(m_mutex);

        m_cv.wait(lock, [this] {
            return m_closed || !m_queue.empty();
        });

        if (!m_queue.empty()) {
            T value = std::move(m_queue.front());
            m_queue.pop();
            return value;
        }

        return std::unexpected{PopFailure::Closed};
    }

    void close() {
        {
            std::lock_guard lock(m_mutex);
            m_closed = true;
        }
        m_cv.notify_all();
    }

    [[nodiscard]] bool is_closed() const {
        return m_closed;
    }

    template <typename Duration>
    std::optional<T> pop_for(Duration timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);

        // Wait until an item is available, the queue is closed, or the timeout expires
        const bool received = m_cv.wait_for(lock, timeout, [this] {
            return !m_queue.empty() || m_closed;
        });

        if (!received || m_queue.empty()) {
            return std::nullopt; // Timed out or queue closed while empty
        }

        T item = std::move(m_queue.front());
        m_queue.pop();
        return item;
    }

    std::expected<T, PopFailure> pop_until(std::chrono::steady_clock::time_point deadline) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait_until(lock, deadline, [this] {
            return !m_queue.empty() || m_closed;
        });

        // Always drain pending items first
        if (!m_queue.empty()) {
            T item = std::move(m_queue.front());
            m_queue.pop();
            return item;
        }

        // Atomically determine why we woke up without an item
        if (m_closed) {
            return std::unexpected(PopFailure::Closed);
        }

        return std::unexpected(PopFailure::Timeout);
    }
private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<T> m_queue;
    bool m_closed = false;
};

#endif //THREADSAFEQUEUE_H
