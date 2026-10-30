#pragma once
#include <filesystem>
#include <variant>
#include <string>
#include <cstdint>

enum class FileChangeFlags : uint8_t {
    None     = 0,
    Init     = 1 << 0,
    Created  = 1 << 1,
    Modified = 1 << 2,
    Renamed  = 1 << 3,
};

constexpr FileChangeFlags operator|(FileChangeFlags a, FileChangeFlags b) noexcept {
    return static_cast<FileChangeFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr FileChangeFlags operator&(FileChangeFlags a, FileChangeFlags b) noexcept {
    return static_cast<FileChangeFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
constexpr FileChangeFlags& operator|=(FileChangeFlags& a, FileChangeFlags b) noexcept {
    return a = (a | b);
}
constexpr bool hasFlag(FileChangeFlags mask, FileChangeFlags flag) noexcept {
    return (mask & flag) != FileChangeFlags::None;
}

struct FileEvent {
    enum class Kind : uint8_t {
        Init,
        Created,
        Modified,
        Removed,
        Renamed
    };

    Kind kind;
    std::filesystem::path path;
    std::filesystem::path oldPath;

    // Factory methods
    static FileEvent init(std::filesystem::path p) {
        return { Kind::Init, std::move(p), {} };
    }
    static FileEvent created(std::filesystem::path p) {
        return { Kind::Created, std::move(p), {} };
    }
    static FileEvent modified(std::filesystem::path p) {
        return { Kind::Modified, std::move(p), {} };
    }
    static FileEvent removed(std::filesystem::path p) {
        return { Kind::Removed, std::move(p), {} };
    }
    static FileEvent renamed(std::filesystem::path from, std::filesystem::path to) {
        return { Kind::Renamed, std::move(to), std::move(from) };
    }

    [[nodiscard]] FileChangeFlags toChangeFlag() const noexcept {
        switch (kind) {
            case Kind::Init:     return FileChangeFlags::Init;
            case Kind::Created:  return FileChangeFlags::Created;
            case Kind::Modified: return FileChangeFlags::Modified;
            case Kind::Renamed:  return FileChangeFlags::Renamed;
            case Kind::Removed:  return FileChangeFlags::None;
        }
        return FileChangeFlags::None;
    }
};
// system control message
struct BufferOverflow {};

using WatcherMessage = std::variant<FileEvent, BufferOverflow>;

struct FileReady {
    std::filesystem::path path;
    FileChangeFlags events = FileChangeFlags::None;
};
