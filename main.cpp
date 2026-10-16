#include <iostream>
#include <string>
#include <thread>
#include <variant>
#include "src/core/FileEvent.h"
#include "src/core/SettleDetector.h"
#include "src/core/ThreadSafeQueue.h"
#include "src/platform/windows/WindowsWatcher.h"

template <class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

std::string describe(const FileEvent& event) {
    return std::visit(overloaded{
        [](const Created& e)  { return "Created: " + e.path.string(); },
        [](const Modified& e) { return "Modified: " + e.path.string(); },
        [](const Removed& e) { return "Removed: " + e.path.string(); },
        [](const Renamed& e) { return "Renamed: from " + e.from.string() + "   to " + e.to.string(); },
        [](const Overflow& e) -> std::string { return "Overflow error, currently unhandled"; },
    }, event);
}

void describe2(const FileEvent& event) {
    std::cout << describe(event) << std::endl;
}

int main() {
    auto lang = "C++";
    std::cout << "Hello and welcome to " << lang << "!\n";
    WindowsWatcher watcher("C:/Saxion/Courses/C++/Testing");
    ThreadSafeQueue<FileEvent> actions_queue;

    std::jthread producer_thread ([&](const std::stop_token& token) {
        auto expect = watcher.start([&actions_queue](FileEvent event) {
            if (!actions_queue.push(std::move(event))) {
                std::cerr << "Cannot push to action queue" << '\n';
            }
        }, token);
        if (!expect) {
            std::cerr << "Watcher failed: " << expect.error() << '\n';
        }
        actions_queue.close();
    });

    std::jthread consumer_thread([&](const std::stop_token& token) {
        constexpr auto quietPeriod = std::chrono::milliseconds(500);
        SettleDetector detector(quietPeriod);

        while (!token.stop_requested()) {
            auto event_opt = actions_queue.pop_for(quietPeriod);
            const auto now = std::chrono::steady_clock::now();
            if (event_opt) {
                detector.onEvent(*event_opt, now);
            }
            for (
                auto ready_files = detector.collectReady(now);
                const auto&[path] : ready_files) {
                std::cout << "[FileReady] Ready for processing: " << path << '\n';
            }
            // If the queue was closed and is now completely empty, exit
            if (!event_opt && actions_queue.is_closed() && detector.empty()) {
                break;
            }
            // describe2(*event_opt);
        }
    });

    std::cout << "Press Enter to stop...\n";
    std::cin.get();

    std::cout << "Stopping app...\n";

    producer_thread.request_stop();
    actions_queue.close();
    consumer_thread.request_stop();
    return 0;
}