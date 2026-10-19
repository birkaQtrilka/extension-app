
#include "EventProcessor.h"

#include <iostream>
#include <system_error>
#include <unordered_set>
#include <utility>
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
    std::cout << describe(event) << '\n';
}

EventProcessor::EventProcessor(
    ThreadSafeQueue<FileEvent>& queue,
    SettleDetector detector,
    RuleEngine ruleEngine,
    std::filesystem::path watchDir
)
    : m_queue(queue)
    , m_detector(std::move(detector))
    , m_ruleEngine(std::move(ruleEngine))
    , m_watchDir(std::move(watchDir))
{}

void EventProcessor::run(const std::stop_token& token) {
    // Initial reconciliation
    auto startupTime = std::chrono::steady_clock::now();
    rescanDirectory(startupTime, FileEvent::init);

    while (!token.stop_requested()) {
        auto now = std::chrono::steady_clock::now();

        processReadyFiles(now);

        auto deadline = m_detector.nextDeadline();
        auto result = deadline
            ? m_queue.pop_until(*deadline)
            : m_queue.pop();

        if (!result) {
            if (result.error() == PopFailure::Closed) {
                // Queue closed: discard and exit
                break;
            }

            // Timeout: let processReadyFiles() pick up ready files
            continue;
        }

        now = std::chrono::steady_clock::now();

        if (result->kind == EventKind::Overflow) {
            std::cout << "[Rescan] Buffer overflow detected. Rescanning "
                      << m_watchDir << "...\n";

            rescanDirectory(now, FileEvent::modified);
        } else {
            m_detector.onEvent(*result, now);
            std::cout << describe(*result) << '\n';
        }
    }
}

void EventProcessor::processReadyFiles(
    std::chrono::steady_clock::time_point now
) {
    for (const auto& readyFile : m_detector.collectReady(now)) {
        std::cout << "[FileReady] Ready: " << readyFile.path << '\n';

        const auto expect = m_ruleEngine.executeActions(readyFile);

        if (!expect) {
            const auto& [actionType, targetPath, ec, message] =
                expect.error();

            std::cerr << "[Action Failed] " << actionType
                      << " on " << readyFile.path.string()
                      << " -> " << targetPath.string()
                      << ": " << message
                      << " (" << ec.message() << ")\n";
        }
    }
}

void EventProcessor::rescanDirectory(
    std::chrono::steady_clock::time_point now,
    const std::function<FileEvent(std::filesystem::path)>& eventCreator
) {
    std::unordered_set<std::filesystem::path> currentFiles;
    std::error_code ec;

    const auto iter = std::filesystem::recursive_directory_iterator(
        m_watchDir,
        std::filesystem::directory_options::skip_permission_denied,
        ec
    );

    if (ec) {
        std::cerr << "[Rescan Error] Failed to open root directory: "
                  << ec.message() << '\n';
        return;
    }

    for (const auto& entry : iter) {
        if (std::error_code status_ec; entry.is_regular_file(status_ec)) {
            const auto& path = entry.path();

            currentFiles.insert(path);
            m_detector.onEvent(eventCreator(path), now);
        }
    }

    m_detector.pruneMissing(currentFiles);

    std::cout << "[Rescan] Completed. Reconciled "
              << currentFiles.size() << " files.\n";
}
