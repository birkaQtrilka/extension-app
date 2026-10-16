#include <iostream>
#include <string>
#include <thread>
#include <variant>
#include "src/core/FileEvent.h"
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
        [](const Overflow& e) { return "Overflow error, currently unhandled"; },
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
            actions_queue.push(std::move(event));
        }, token);
        if (!expect) {
            std::cerr << "Watcher failed: " << expect.error() << '\n';
            actions_queue.close();
        }
    });

    std::jthread consumer_thread([&](const std::stop_token& token) {
        while (!token.stop_requested()) {
            auto event_opt = actions_queue.pop();
            if (!event_opt) {
                break;
            }
            describe2(*event_opt);
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