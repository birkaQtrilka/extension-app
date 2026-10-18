#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <unordered_set>
#include <filesystem>

#include "core/ConfigParser.h"
#include "core/FileEvent.h"
#include "core/RuleEngine.h"
#include "core/SettleDetector.h"
#include "core/ThreadSafeQueue.h"
#include "platform/windows/WindowsWatcher.h"
#include "platform/windows/WindowsFileProbe.h"

std::string describe(const FileEvent& event) {
    switch (event.kind) {
        case EventKind::Created:  return "Created: " + event.path.string();
        case EventKind::Modified: return "Modified: " + event.path.string();
        case EventKind::Removed:  return "Removed: " + event.path.string();
        case EventKind::Renamed:  return "Renamed: from " + event.oldPath.string() + " to " + event.path.string();
        case EventKind::Overflow: return "Overflow error detected";
        default:                  return "Unknown event";
    }
}

void describe2(const FileEvent& event) {
    std::cout << describe(event) << std::endl;
}

void initialScanDirectory(
    const std::filesystem::path& root,
    SettleDetector& detector,
    const std::chrono::steady_clock::time_point now
) {
    std::cout << "[InitScan] Scanning existing files in " << root << "...\n";

    std::error_code ec;
    const auto iter = std::filesystem::recursive_directory_iterator(
        root,
        std::filesystem::directory_options::skip_permission_denied,
        ec
    );

    if (ec) {
        std::cerr << "[InitScan Error] Failed to open root directory: " << ec.message() << '\n';
        return;
    }

    std::size_t count = 0;
    for (const auto& entry : iter) {
        if (std::error_code status_ec; entry.is_regular_file(status_ec)) {
            const auto& path = entry.path();
            detector.onEvent(FileEvent::init(path), now);
            ++count;
        }
    }

    std::cout << "[InitScan] Completed. Discovered " << count << " existing files.\n";
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
            detector.onEvent(FileEvent::modified(path), now);
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
        ConfigParser parser("rules.json");
        auto parsedRules = parser.parse();
        if (!parsedRules) {
            std::cerr << "Failed to parse rules.json\n";
            return;
        }
        RuleEngine rule_engine(std::move(parsedRules.value()));
        auto startupTime = std::chrono::steady_clock::now();
        initialScanDirectory(watcher.getWatchedDir(), detector, startupTime);

        while (!token.stop_requested()) {
            auto now = std::chrono::steady_clock::now();

            // Emit files that settled naturally during the quiet period
            for (const auto& readyFile : detector.collectReady(now)) {
                std::cout << "[FileReady] Ready: " << readyFile.path << '\n';
                const auto expect = rule_engine.executeActions(readyFile);
                if (!expect) {
                    const auto&[actionType, targetPath, ec, message] = expect.error();
                    std::cerr << "[Action Failed] " << actionType
                          << " on " + readyFile.path.string() << " -> " << targetPath.string()
                          << ": " << message << " (" << ec.message() << ")\n";
                }
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
            if (result->kind == EventKind::Overflow) {
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