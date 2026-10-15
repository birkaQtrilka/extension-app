#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include "src/core/FileEvent.h"

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

int main() {
    // TIP Press <shortcut actionId="RenameElement"/> when your caret is at the
    // <b>lang</b> variable name to see how CLion can help you rename it.
    auto lang = "C++";
    std::cout << "Hello and welcome to " << lang << "!\n";
    std::optional<FileEvent> instr;
    std::string command;
    std::cin >> command;

    if (command == "created") {
        instr = Created{"file.txt"};
    } else if (command == "modified") {
        instr = Renamed{"file.txt", "file2.txt"};
    }
    if (instr.has_value()) {
        std::cout << describe(instr.value());
    }
    else {
        std::cout << "No recognized instruction\n";
    }

    return 0;
}

// TIP See CLion help at <a
// href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>.
//  Also, you can try interactive lessons for CLion by selecting
//  'Help | Learn IDE Features' from the main menu.