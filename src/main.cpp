#include <iostream>
#include <chrono>
#include <filesystem>

#include "core/FileWatcherApp.h"
#include "core/SettingsParser.h"
#include "core/ThreadSafeQueue.h"
#include "platform/windows/WindowsWatcher.h"
#include "platform/windows/WindowsFileProbe.h"

int main() {
    SettingsParser settings_parser("settings.json");
    auto expect = settings_parser.parse();
    if (!expect) {
        std::cerr << expect.error() << std::endl;
        return 1;
    }
    AppConfig config = *expect;

    FileWatcherApp<WindowsWatcher, WindowsFileProbe> app(std::move(config), WindowsFileProbe{});

    if (!app.init()) {
        std::cerr << "Initialization failed. Exiting...\n";
        return 1;
    }

    app.start();
    std::cout << "Press Enter to stop...\n\n\n";

    std::cin.get();

    app.stop();

    return 0;
}