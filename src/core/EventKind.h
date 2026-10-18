#pragma once
#include <cstdint>

enum class EventKind : uint8_t {
    None     = 0,
    Created  = 1 << 0,
    Modified = 1 << 1,
    Removed  = 1 << 2,
    Renamed  = 1 << 3,
    Overflow = 1 << 4,
    Init = 1 << 5,
};

constexpr EventKind operator|(EventKind a, EventKind b) noexcept {
    return static_cast<EventKind>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr EventKind operator&(EventKind a, EventKind b) noexcept {
    return static_cast<EventKind>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
constexpr EventKind& operator|=(EventKind& a, EventKind b) noexcept {
    return a = (a | b);
}
constexpr bool hasFlag(EventKind mask, EventKind flag) noexcept {
    return (mask & flag) != EventKind::None;
}