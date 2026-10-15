//
// Created by Birka on 15/10/2026.
//

#ifndef WINDOWSWATCHER_H
#define WINDOWSWATCHER_H
#include <windows.h>

#include "../../core/IFolderWatcher.h"

class WindowsWatcher final : public IFolderWatcher {
public:
    explicit WindowsWatcher(std::string dir_path);
    std::expected<void, std::error_code> start(EventCallback onEvent, std::stop_token stop) override;

    void ProcessEvents(const EventCallback &onEvent, FILE_NOTIFY_INFORMATION *info);

private:
    std::string m_dir_path;
    std::wstring m_old_name_for_rename;
};




#endif //WINDOWSWATCHER_H
