#pragma once
#include <chrono>
#include <unordered_map>
#include <filesystem>
#include <unordered_set>
#include <vector>
#include <optional>

#include "FileEvent.h"
#include "IFileProbe.h"

using TimePoint = std::chrono::steady_clock::time_point;

class SettleDetector {
public:
    // This internal struct tracks files that are currently debouncing
    struct PendingEntry {
        TimePoint lastSeen;
        FileChangeFlags accumulatedEvents = FileChangeFlags::None;
    };

    explicit SettleDetector(const std::chrono::milliseconds quietPeriod, const IFileProbe& probe)
        : m_quietPeriod(quietPeriod), m_probe(probe) {}

    void onEvent(const FileEvent& event, TimePoint now);
    std::vector<FileReady> collectReady(TimePoint now);
    
    [[nodiscard]] bool empty() const noexcept { return m_pending.empty(); }
    void pruneMissing(const std::unordered_set<std::filesystem::path>& existingPaths);
    [[nodiscard]] std::optional<TimePoint> nextDeadline() const;

private:
    std::chrono::milliseconds m_quietPeriod;
    const IFileProbe& m_probe;
    std::unordered_map<std::filesystem::path, PendingEntry> m_pending;
};