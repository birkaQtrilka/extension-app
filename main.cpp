#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include "src/core/FileEvent.h"
#include "src/platform/cross/WindowsWatcher.h"

// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or
// click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.

template <class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

std::string describe(const FileEvent& event) {
    return std::visit(overloaded{
        [](const Created& e)  { return "Created: " + e.path.string(); },
        [](const Modified& e) { return "Modified: " + e.path.string(); },
        [](const Removed& e) { return "Removed: " + e.path.string(); },
        [](const Renamed& e) { return "Modified: from" + e.from.string() + "to " + e.to.string(); },
    }, event);
}
void describe2(const FileEvent& event) {
    std::cout << describe(event) << std::endl;
}
int main() {
    auto lang = "C++";
    std::cout << "Hello and welcome to " << lang << "!\n";
    WindowsWatcher watcher("C:/Saxion/Courses/C++/Testing");
    auto token = std::stop_token();
    watcher.start(describe2, token);

    return 0;
}

// TIP See CLion help at <a
// href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>.
//  Also, you can try interactive lessons for CLion by selecting
//  'Help | Learn IDE Features' from the main menu.