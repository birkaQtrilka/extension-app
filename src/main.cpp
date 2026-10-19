#include <iostream>
#include <chrono>
#include <filesystem>

#include "core/FileWatcherApp.h"
#include "core/ThreadSafeQueue.h"
#include "platform/windows/WindowsWatcher.h"
#include "platform/windows/WindowsFileProbe.h"

int main() {
    AppConfig config{
        .watchPath = "C:/Saxion/Courses/C++/Testing",
        .configPath = "rules.json",
        .quietPeriod = std::chrono::milliseconds(500)
    };

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