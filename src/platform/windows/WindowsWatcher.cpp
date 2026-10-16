#include "WindowsWatcher.h"

#include <array>
#include <iostream>
#include <utility>
#include <windows.h>
#include "HandleCloser.h"

WindowsWatcher::WindowsWatcher(std::filesystem::path dir_path)
    : m_dir_path(std::move(dir_path))
{
}

std::expected<void, std::error_code> WindowsWatcher::start(const EventCallback& onEvent,const std::stop_token stop)
{
    // Must outlive handles (destroyed last)
    alignas(DWORD) std::array<std::byte, 64 * 1024> buffer{};
    OVERLAPPED ov{};

    const HANDLE rawDirHandle = CreateFileA(
        m_dir_path.string().c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr
    );

    if (rawDirHandle == INVALID_HANDLE_VALUE) {
        return std::unexpected(std::error_code(GetLastError(), std::system_category()));
    }
    const UniqueHandle hDir{rawDirHandle};

    const HANDLE rawIoEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!rawIoEvent) {
        return std::unexpected(std::error_code(GetLastError(), std::system_category()));
    }
    const UniqueHandle ioEvent{rawIoEvent};

    const HANDLE rawStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!rawStopEvent) {
        return std::unexpected(std::error_code(GetLastError(), std::system_category()));
    }
    const UniqueHandle stopEvent{rawStopEvent};

    ov.hEvent = ioEvent.get();

    // When a stop is requested, signal the stop event.
    std::stop_callback onStop(stop, [&stopEvent] {
        SetEvent(stopEvent.get());
    });

    constexpr DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME |
                             FILE_NOTIFY_CHANGE_DIR_NAME |
                             FILE_NOTIFY_CHANGE_ATTRIBUTES |
                             FILE_NOTIFY_CHANGE_SIZE |
                             FILE_NOTIFY_CHANGE_LAST_WRITE |
                             FILE_NOTIFY_CHANGE_CREATION;

    std::cout << "Watching directory: " << m_dir_path << "\n";

    while (!stop.stop_requested()) {
        // Reset the manual-reset I/O event and OVERLAPPED internal state
        ResetEvent(ioEvent.get());
        ov.Internal = 0;
        ov.InternalHigh = 0;
        ov.Offset = 0;
        ov.OffsetHigh = 0;
        ov.Pointer = nullptr;

        const BOOL readOk = ReadDirectoryChangesW(
            hDir.get(),
            buffer.data(),
            static_cast<DWORD>(buffer.size()),
            TRUE, // watch subdirectories
            filter,
            nullptr,
            &ov,
            nullptr
        );

        if (!readOk) {
            if (const DWORD err = GetLastError(); err != ERROR_IO_PENDING) {
                std::cerr << "ReadDirectoryChangesW failed: " << err << "\n";
                return std::unexpected(std::error_code(err, std::system_category()));
            }
        }

        // Wait for either: [0] I/O to finish, or [1] Stop requested
        HANDLE waitHandles[] = { ioEvent.get(), stopEvent.get() };
        const DWORD waitResult = WaitForMultipleObjects(
            static_cast<DWORD>(std::size(waitHandles)),
            waitHandles,
            FALSE, // Wake on ANY event
            INFINITE
        );

        if (waitResult == WAIT_OBJECT_0) {
            DWORD bytesReturned = 0;
            if (!GetOverlappedResult(hDir.get(), &ov, &bytesReturned, FALSE)) {
                const DWORD err = GetLastError();
                if (err == ERROR_OPERATION_ABORTED) {
                    break;
                }
                std::cerr << "GetOverlappedResult failed: " << err << "\n";
                return std::unexpected(std::error_code(err, std::system_category()));
            }

            if (bytesReturned == 0) {
                std::cerr << "Buffer overflow! Some events were lost. Full rescan needed.\n";
                continue;
            }

            auto* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.data());
            ProcessEvents(onEvent, fni);
        }
        else if (waitResult == WAIT_OBJECT_0 + 1) {
            // Tell Windows to cancel the outstanding read
            CancelIoEx(hDir.get(), &ov);

            // Wait (bWait = TRUE) until Windows confirms cancellation.
            // This guarantees the kernel is completely done with buffer and ov.
            DWORD dummyBytes = 0;
            GetOverlappedResult(hDir.get(), &ov, &dummyBytes, TRUE);

            break;
        }
        else {
            const DWORD err = GetLastError();
            std::cerr << "WaitForMultipleObjects failed: " << err << "\n";
            return std::unexpected(std::error_code(err, std::system_category()));
        }
    }

    return {};
}

void WindowsWatcher::ProcessEvents(const EventCallback& onEvent, FILE_NOTIFY_INFORMATION* info) {
    while (true) {
        std::wstring_view nameView(info->FileName, info->FileNameLength / sizeof(WCHAR));

        switch (info->Action) {
            case FILE_ACTION_ADDED:
                onEvent(Created{nameView});
            break;
            case FILE_ACTION_REMOVED:
                onEvent(Removed{nameView});
            break;
            case FILE_ACTION_MODIFIED:
                onEvent(Modified{nameView});
            break;
            case FILE_ACTION_RENAMED_OLD_NAME:
                m_old_name_for_rename = nameView;
            break;
            case FILE_ACTION_RENAMED_NEW_NAME:
                if (!m_old_name_for_rename.empty()) {
                    onEvent(Renamed{m_old_name_for_rename, nameView});
                    m_old_name_for_rename.clear(); // Reset for safety
                } else {
                    // Edge case: A file was moved INTO the watched directory from somewhere else.
                    onEvent(Created{nameView});
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