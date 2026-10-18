#include <iostream>
#include <string>
#include <thread>
#include <variant>
#include <chrono>
#include "src/core/Overloaded.h"
#include "src/core/FileEvent.h"
#include "src/core/SettleDetector.h"
#include "src/core/ThreadSafeQueue.h"
#include "src/platform/windows/WindowsWatcher.h"
#include "src/platform/windows/WindowsFileProbe.h"

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
    const WindowsFileProbe file_probe;
    WindowsWatcher watcher("C:/Saxion/Courses/C++/Testing");
    std::cout << "Watching directory: " << watcher.getWatchedDir() << "\n";

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
        SettleDetector detector(quietPeriod, file_probe);
        bool queue_closed = false;

        while (!token.stop_requested()) {
            // auto event_opt = actions_queue.pop_for(quietPeriod);
            auto now = std::chrono::steady_clock::now();
            for (
                auto ready_files = detector.collectReady(now);
                const auto&[path] : ready_files) {
                std::cout << "[FileReady] Ready: " << path << '\n';
            }
            auto deadline = detector.nextDeadline();
            if (queue_closed) {
                if (!deadline) {
                    break;
                }
                std::this_thread::sleep_until(*deadline);
                continue;
            }
            std::expected<FileEvent, PopFailure> result = deadline
                ? actions_queue.pop_until(*deadline)
                : actions_queue.pop();

            now = std::chrono::steady_clock::now();
            if (result) {
                if (std::holds_alternative<Overflow>(*result)) {
                    rescanDirectory(watcher.getWatchedDir(), detector, now);
                } else {
                    detector.onEvent(*result, now);
                    describe2(*result);
                }
            }
            else {
                if (result.error() == PopFailure::Closed) {
                    queue_closed = true;
                }
                // If PopFailure::Timeout, do nothing: loop restarts and collectReady() emits the file!            }
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