#pragma once
#include <filesystem>
#include "EventKind.h"

struct FileEvent {
    EventKind kind = EventKind::None;
    std::filesystem::path path;     // Target/current path
    std::filesystem::path oldPath;  // Populated only when kind is Renamed ('from')

    // Factory helpers for convenience
    static FileEvent created(std::filesystem::path p) {
        return { EventKind::Created, std::move(p), {} };
    }
    static FileEvent modified(std::filesystem::path p) {
        return { EventKind::Modified, std::move(p), {} };
    }
    static FileEvent removed(std::filesystem::path p) {
        return { EventKind::Removed, std::move(p), {} };
    }
    static FileEvent renamed(std::filesystem::path from, std::filesystem::path to) {
        return { EventKind::Renamed, std::move(to), std::move(from) };
    }
    static FileEvent overflow() {
        return { EventKind::Overflow, {}, {} };
    }
};

// Ready-file after debounce/settling (can even reuse FileEvent, or alias it)
struct FileReady {
    std::filesystem::path path;
    EventKind events = EventKind::None; // Cumulative mask of what happened to it
};