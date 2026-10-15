#include "WindowsWatcher.h"

#include <iostream>
#include <utility>
#include <windows.h>
#include "HandleCloser.h"

WindowsWatcher::WindowsWatcher(std::string  dir_path)
    : m_dir_path(std::move(dir_path))
{
}

std::expected<void, std::error_code> WindowsWatcher::start(EventCallback onEvent, std::stop_token stop)
{
    const HANDLE rawHandle = CreateFileA(
            m_dir_path.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            nullptr
        );

    if (rawHandle == INVALID_HANDLE_VALUE) {
        const auto error = GetLastError();
        return std::unexpected(
            std::error_code(error, std::system_category())
        );
    }
    UniqueHandle hDir{rawHandle};

    alignas(DWORD) std::array<std::byte, 64 * 1024> buffer{};
    DWORD bytesReturned = 0;

    std::cout << "Watching directory: " << m_dir_path << "\n";
    std::stop_callback onStop(stop, [&] { CancelIoEx(hDir.get(), nullptr); });

    while (!stop.stop_requested()) {

        // Read directory changes
        if (ReadDirectoryChangesW(
            hDir.get(),
            buffer.data(),
            buffer.size(),
            TRUE, // watch subdirectories
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_ATTRIBUTES |
            FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE |
            FILE_NOTIFY_CHANGE_CREATION,
            &bytesReturned,
            nullptr,
            nullptr
        )) {
            if (bytesReturned == 0) {
                std::cerr << "Buffer overflow! Some events were lost. Full rescan needed.\n";
                // Optionally push a special "Rescan" event to the queue here
                continue;
            }
            auto* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.data());
            ProcessEvents(onEvent, fni);

        } else {
            const DWORD err = GetLastError();
            if (err == ERROR_OPERATION_ABORTED) {
                // Not an error! The stop token cancelled the I/O cleanly.
                break;
            }
            std::cerr << "ReadDirectoryChangesW failed: " << err << "\n";
            return std::unexpected(std::error_code(err, std::system_category()));
        }
    }

    return {};
}

void WindowsWatcher::ProcessEvents(const EventCallback& onEvent, FILE_NOTIFY_INFORMATION* info) {
    while (true) {
        std::wstring wFileName(info->FileName, info->FileNameLength / sizeof(WCHAR));

        switch (info->Action) {
            case FILE_ACTION_ADDED:
                onEvent(Created{wFileName});
            break;
            case FILE_ACTION_REMOVED:
                onEvent(Removed{wFileName});
            break;
            case FILE_ACTION_MODIFIED:
                onEvent(Modified{wFileName});
            break;
            case FILE_ACTION_RENAMED_OLD_NAME:
                m_old_name_for_rename = wFileName;
            break;
            case FILE_ACTION_RENAMED_NEW_NAME:
                if (!m_old_name_for_rename.empty()) {
                    onEvent(Renamed{m_old_name_for_rename, wFileName});
                    m_old_name_for_rename.clear(); // Reset for safety
                } else {
                    // Edge case: A file was moved INTO the watched directory from somewhere else.
                    // Windows only gives us NEW_NAME. We should treat it as a Creation.
                    onEvent(Created{wFileName});
                }
            break;
            default:
                break;
        }

        if (info->NextEntryOffset == 0) {
            break;
        }

        // Advance the pointer to the next struct in the buffer
        info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
            reinterpret_cast<char*>(info) + info->NextEntryOffset
        );
    }
}