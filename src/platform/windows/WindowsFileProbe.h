//
// Created by Birka on 18/10/2026.
//
#pragma once

#include <windows.h>
#include <filesystem>
#include "../../core/IFileProbe.h"

class WindowsFileProbe final : public IFileProbe {
public:
    [[nodiscard]] bool isRegularFile(const std::filesystem::path& path) const override {
        std::error_code ec;
        return std::filesystem::is_regular_file(path, ec);
    }

    [[nodiscard]] bool isStable(const std::filesystem::path& path) const override {
        // Attempt to open the file with zero sharing (exclusive mode).
        // If ANY other process has an open handle, this call fails immediately.
        HANDLE hFile = CreateFileW(
            path.c_str(),
            GENERIC_READ,
            0,                      // dwShareMode = 0 Prevents all other access
            nullptr,                // Security attributes
            OPEN_EXISTING,          // Only open if it already exists
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (hFile == INVALID_HANDLE_VALUE) {
            // Fails with ERROR_SHARING_VIOLATION
            return false;
        }

        CloseHandle(hFile);
        return true;
    }
};