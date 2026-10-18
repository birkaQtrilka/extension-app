//
// Created by Birka on 16/10/2026.
//

#ifndef SETTTLEDETECTOR_H
#define SETTTLEDETECTOR_H
#include <chrono>
#include <unordered_map>
#include <filesystem>
#include <unordered_set>

#include "FileEvent.h"
#include "IFileProbe.h"
using TimePoint = std::chrono::steady_clock::time_point;

class SettleDetector {
public:
    explicit SettleDetector(std::chrono::milliseconds quietPeriod, const IFileProbe& probe)
        : m_quietPeriod(quietPeriod), m_probe(probe) {}

    void onEvent(const FileEvent& event, TimePoint now);
    std::vector<FileReady> collectReady(TimePoint now);
    [[nodiscard]] bool empty() const noexcept { return m_lastSeen.empty(); }
    void pruneMissing(const std::unordered_set<std::filesystem::path>& existingPaths);
    // Returns the exact moment the earliest pending file will be ready,
    // or std::nullopt if no files are being tracked.
    [[nodiscard]] std::optional<std::chrono::steady_clock::time_point> nextDeadline() const;

private:
    const IFileProbe& m_probe;
    std::chrono::milliseconds m_quietPeriod;
    std::unordered_map<std::filesystem::path,
        TimePoint> m_lastSeen;
};
#endif //SETTTLEDETECTOR_H
