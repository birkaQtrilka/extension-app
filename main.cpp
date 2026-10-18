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

void rescanDirectory(
    const std::filesystem::path& root,
    SettleDetector& detector,
    const std::chrono::steady_clock::time_point now
) {
    std::cout << "[Rescan] Buffer overflow detected. Rescanning " << root << "...\n";

    std::unordered_set<std::filesystem::path> currentFiles;
    std::error_code ec;

    const auto iter = std::filesystem::recursive_directory_iterator(
        root,
        std::filesystem::directory_options::skip_permission_denied,
        ec
    );

    if (ec) {
        std::cerr << "[Rescan Error] Failed to open root directory: " << ec.message() << '\n';
        return;
    }

    for (const auto& entry : iter) {
        if (std::error_code status_ec; entry.is_regular_file(status_ec)) {
            const auto& path = entry.path();
            currentFiles.insert(path);

            // Re-affirm existence: treat as seen right now
            detector.onEvent(Modified{path}, now);
        }
    }

    // Remove any paths that were deleted during the overflow
    detector.pruneMissing(currentFiles);

    std::cout << "[Rescan] Completed. Reconciled " << currentFiles.size() << " files.\n";
}

int main() {
    const auto lang = "C++";
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
                if (std::holds_alternative<Overflow>(*event_opt)) {
                    rescanDirectory(watcher.getWatchedDir(), detector, now);
                } else {
                    detector.onEvent(*event_opt, now);
                    describe2(*event_opt);
                }
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