//
// Created by Birka on 16/10/2026.
//

#ifndef SETTTLEDETECTOR_H
#define SETTTLEDETECTOR_H
#include <chrono>
#include <unordered_map>
#include <filesystem>

#include "FileEvent.h"
using TimePoint = std::chrono::steady_clock::time_point;

class SettleDetector {
public:
    explicit SettleDetector(const std::chrono::milliseconds quietPeriod)
        : m_quietPeriod(quietPeriod) {}

    void onEvent(const FileEvent& event, TimePoint now);
    std::vector<FileReady> collectReady(TimePoint now);
    [[nodiscard]] bool empty() const noexcept { return m_lastSeen.empty(); }

private:
    std::chrono::milliseconds m_quietPeriod;
    std::unordered_map<std::filesystem::path,
        TimePoint> m_lastSeen;
};
#endif //SETTTLEDETECTOR_H
