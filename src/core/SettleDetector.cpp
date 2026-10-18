//
// Created by Birka on 16/10/2026.
//
#include "SettleDetector.h"
#include <variant>
#include "Overloaded.h"
void SettleDetector::onEvent(const FileEvent& event, std::chrono::steady_clock::time_point now) {
    std::visit(overloaded{
        [&](const Created& e) {
            m_lastSeen[e.path] = now;
        },
        [&](const Modified& e) {
            m_lastSeen[e.path] = now;
        },
        [&](const Removed& e) {
            // Cancel any pending notification for a file that was deleted
            m_lastSeen.erase(e.path);
        },
        [&](const Renamed& e) {
            // Cancel old temporary path, start debounce on the new destination path
            m_lastSeen.erase(e.from);
            m_lastSeen[e.to] = now;
        },
        [&](const Overflow&) { },
    }, event);
}

std::vector<FileReady> SettleDetector::collectReady(std::chrono::steady_clock::time_point now) {
    std::vector<FileReady> ready;

    std::erase_if(m_lastSeen, [&](auto& entry) {
        auto& [path, lastSeen] = entry;
        if (now - lastSeen < m_quietPeriod) {
            return false; // Keep waiting
        }
        // TODO: erase from map do not emit fileready if file is in "action_performed" state
        // for example, if an action moves file within the same watched directory, it should not infinitely loop
        if (!m_probe.isRegularFile(path)) {
            return true; // Erase from map, do NOT emit FileReady
        }
        if (!m_probe.isStable(path)) {
            // Stalled download / still locked by writer:
            // Refresh timestamp and let it sit for another quietPeriod
            lastSeen = now;
            return false; // Keep in map
        }
        ready.push_back(FileReady{path});
        return true; // erase from map and emit
    });

    return ready;
}

void SettleDetector::pruneMissing(const std::unordered_set<std::filesystem::path> &existingPaths) {
    std::erase_if(m_lastSeen, [&](const auto& pair) {
        return !existingPaths.contains(pair.first);
    });
}

std::optional<std::chrono::steady_clock::time_point> SettleDetector::nextDeadline() const {
    if (m_lastSeen.empty()) {
        return std::nullopt;
    }

    // Find the file that was seen earliest
    const auto earliestIt = std::min_element(
        m_lastSeen.begin(), m_lastSeen.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        }
    );

    return earliestIt->second + m_quietPeriod;
}

