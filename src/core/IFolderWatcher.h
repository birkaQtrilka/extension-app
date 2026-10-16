//
// Created by Birka on 15/10/2026.
//

#ifndef IFOLDERWATCHER_H
#define IFOLDERWATCHER_H
#include <expected>
#include <format>
#include <functional>
#include <stop_token>
#include "FileEvent.h"

struct WatchError {
    const char* operation;       // e.g. "ReadDirectoryChangesW"
    std::error_code code;        // Windows error code + system_category

    std::string message() const {
        return std::format("{} failed: {} ({})", operation, code.message(), code.value());
    }
    friend std::ostream& operator<<(std::ostream& os, const WatchError& err) {
        return os << err.operation << " failed [" << err.code.value() << "]: "
                  << err.code.message();
    }
};


template <>
struct std::formatter<WatchError> : std::formatter<std::string_view> {
    auto format(const WatchError& err, std::format_context& ctx) const {
        std::string s = std::format("{} failed [{}]: {}",
                                    err.operation,
                                    err.code.value(),
                                    err.code.message());
        return std::formatter<std::string_view>::format(s, ctx);
    }
};

class IFolderWatcher {
public:
    virtual ~IFolderWatcher() = default;
    using EventCallback = std::function<void(FileEvent)>;
    virtual std::expected<void, WatchError>
        start(const EventCallback& onEvent, std::stop_token stop) = 0;
};


#endif //IFOLDERWATCHER_H
