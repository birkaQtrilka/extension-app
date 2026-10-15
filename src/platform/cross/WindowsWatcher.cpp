#include "WindowsWatcher.h"

#include <iostream>
#include <utility>
#include <windows.h>

WindowsWatcher::WindowsWatcher(std::string  dir_path)
    : m_dir_path(std::move(dir_path))
{
}

void WindowsWatcher::start(EventCallback onEvent, std::stop_token stop)
{
    HANDLE hDir = CreateFileA(
            m_dir_path.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL
        );

    if (hDir == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening directory: " << GetLastError() << "\n";
        return;
    }

    char buffer[1024];
    DWORD bytesReturned;

    std::cout << "Watching directory: " << m_dir_path << "\n";

    while (true) {
        // Read directory changes
        if (ReadDirectoryChangesW(
            hDir,
            &buffer,
            sizeof(buffer),
            TRUE, // watch subdirectories
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_ATTRIBUTES |
            FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE |
            FILE_NOTIFY_CHANGE_CREATION,
            &bytesReturned,
            NULL,
            NULL
        )) {
            auto* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
            ProcessEvents(onEvent, fni);

        } else {
            std::cerr << "ReadDirectoryChangesW failed: " << GetLastError() << "\n";
            break;
        }
    }

    CloseHandle(hDir);
}

void WindowsWatcher::ProcessEvents(const EventCallback& onEvent, FILE_NOTIFY_INFORMATION* info) {
    std::wstring oldNameForRename;

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
                oldNameForRename = wFileName;
            break;
            case FILE_ACTION_RENAMED_NEW_NAME:
                if (!oldNameForRename.empty()) {
                    onEvent(Renamed{oldNameForRename, wFileName});
                    oldNameForRename.clear(); // Reset for safety
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