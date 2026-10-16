//
// Created by Birka on 16/10/2026.
//
#include "SettleDetector.h"
#include <variant>

// Helper for std::visit
template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };

void SettleDetector::onEvent(const FileEvent& event, std::chrono::steady_clock::time_point now) {
    std::visit(Overloaded{
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
        [&](const Overflow&) {
            // Continuity broken: clear unconfirmed state
            m_lastSeen.clear();
        },
    }, event);
}

std::vector<FileReady> SettleDetector::collectReady(std::chrono::steady_clock::time_point now) {
    std::vector<FileReady> ready;

    // C++20 std::erase_if on maps cleanly removes elements while iterating
    std::erase_if(m_lastSeen, [&](const auto& entry) {
        const auto& [path, lastSeen] = entry;
        if (now - lastSeen >= m_quietPeriod) {
            ready.push_back(FileReady{path});
            return true; // Remove from map
        }
        return false; // Keep waiting
    });

    return ready;
}