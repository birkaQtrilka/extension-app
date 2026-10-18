#include <iostream>
#include <string>
#include <thread>
#include <variant>
#include <chrono>
#include <unordered_set>
#include <filesystem>
#include "core/Overloaded.h"
#include "core/FileEvent.h"
#include "core/SettleDetector.h"
#include "core/ThreadSafeQueue.h"
#include "platform/windows/WindowsWatcher.h"
#include "platform/windows/WindowsFileProbe.h"

std::string describe(const FileEvent& event) {
    return std::visit(overloaded{
        [](const Created& e)  { return "Created: " + e.path.string(); },
        [](const Modified& e) { return "Modified: " + e.path.string(); },
        [](const Removed& e)  { return "Removed: " + e.path.string(); },
        [](const Renamed& e)  { return "Renamed: from " + e.from.string() + " to " + e.to.string(); },
        [](const Overflow&)   { return std::string{"Overflow error, currently unhandled"}; },
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
            detector.onEvent(Modified{path}, now);
        }
    }

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

    std::jthread producer_thread([&](const std::stop_token& token) {
        auto expect = watcher.start([&actions_queue](FileEvent event) {
            if (!actions_queue.push(std::move(event))) {
                std::cerr << "Cannot push to action queue\n";
            }
        }, token);
        if (!expect) {
            std::cerr << "Watcher failed: " << expect.error() << '\n';
        }
        // Producer is done; signal the queue that no more events will arrive.
        actions_queue.close();
    });

    std::jthread consumer_thread([&](const std::stop_token& token) {
        constexpr auto quietPeriod = std::chrono::milliseconds(500);
        SettleDetector detector(quietPeriod, file_probe);

        while (!token.stop_requested()) {
            auto now = std::chrono::steady_clock::now();

            // Emit files that settled naturally during the quiet period
            for (const auto& [path] : detector.collectReady(now)) {
                std::cout << "[FileReady] Ready: " << path << '\n';
            }

            auto deadline = detector.nextDeadline();
            std::expected<FileEvent, PopFailure> result = deadline
                ? actions_queue.pop_until(*deadline)
                : actions_queue.pop();

            if (!result) {
                if (result.error() == PopFailure::Closed) {
                    // Queue is closed and drained of raw events.
                    // Per policy: discard pending files in detector and exit.
                    break;
                }
                // Timeout: Loop restarts and collectReady() emits settled files.
                continue;
            }

            now = std::chrono::steady_clock::now();
            if (std::holds_alternative<Overflow>(*result)) {
                rescanDirectory(watcher.getWatchedDir(), detector, now);
            } else {
                detector.onEvent(*result, now);
                describe2(*result);
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