
#pragma once

#include <filesystem>
#include <functional>
#include <stop_token>

#include "FileEvent.h"
#include "RuleEngine.h"
#include "SettleDetector.h"
#include "ThreadSafeQueue.h"

class EventProcessor {
public:
    EventProcessor(
        ThreadSafeQueue<WatcherMessage>& queue,
        SettleDetector detector,
        RuleEngine ruleEngine,
        std::filesystem::path watchDir
    );

    // The main loop executed by the consumer thread
    void run(const std::stop_token& token);

private:
    void processReadyFiles(std::chrono::steady_clock::time_point now);

    void rescanDirectory(std::chrono::steady_clock::time_point now, FileEvent::Kind kind);

    ThreadSafeQueue<WatcherMessage>& m_queue;
    SettleDetector m_detector;
    RuleEngine m_ruleEngine;
    std::filesystem::path m_watchDir;
};
