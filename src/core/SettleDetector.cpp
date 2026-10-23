#include "SettleDetector.h"

void SettleDetector::onEvent(const FileEvent& event, TimePoint now) {
    switch (event.kind) {
        case FileEvent::Kind::Init:
        case FileEvent::Kind::Created:
        case FileEvent::Kind::Modified: {
            auto& entry = m_pending[event.path];
            entry.lastSeen = now;
            entry.accumulatedEvents |= event.toChangeFlag();
            break;
        }

        case FileEvent::Kind::Removed: {
            m_pending.erase(event.path);
            break;
        }

        case FileEvent::Kind::Renamed: {
            m_pending.erase(event.oldPath);
            auto& entry = m_pending[event.path];
            entry.lastSeen = now;
            entry.accumulatedEvents |= FileChangeFlags::Renamed;
            break;
        }
        // Notice: NO Overflow case needed here at all!
    }
}

std::vector<FileReady> SettleDetector::collectReady(TimePoint now) {
    std::vector<FileReady> ready;

    std::erase_if(m_pending, [&](auto& pair) {
        auto& [path, data] = pair;

        // Still within the debounce quiet period
        if (now - data.lastSeen < m_quietPeriod) {
            return false;
        }

        // File disappeared or is not a regular file
        if (!m_probe.isRegularFile(path)) {
            return true; // Erase from tracking, do NOT emit
        }

        // File is still being held/written by another process (e.g. large file copy)
        if (!m_probe.isStable(path)) {
            data.lastSeen = now; // Push back timer
            return false;
        }

        // File has settled and is safe to process!
        ready.push_back(FileReady{
            .path = path,
            .events = data.accumulatedEvents
        });

        return true; // Erase from tracking and emit
    });

    return ready;
}

void SettleDetector::pruneMissing(const std::unordered_set<std::filesystem::path>& existingPaths) {
    std::erase_if(m_pending, [&](const auto& pair) {
        return !existingPaths.contains(pair.first);
    });
}

std::optional<TimePoint> SettleDetector::nextDeadline() const {
    if (m_pending.empty()) {
        return std::nullopt;
    }

    const auto earliestIt = std::min_element(
        m_pending.begin(), m_pending.end(),
        [](const auto& a, const auto& b) {
            return a.second.lastSeen < b.second.lastSeen;
        }
    );

    return earliestIt->second.lastSeen + m_quietPeriod;
}