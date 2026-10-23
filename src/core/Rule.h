//
// Created by Birka on 18/10/2026.
//

#ifndef RULE_H
#define RULE_H
#include <regex>
#include "IAction.h"
#include <filesystem>

#include "FileEvent.h"

struct Rule {
    FileChangeFlags triggerEvents = FileChangeFlags::None;
    std::vector<std::regex> compiledPatterns;
    std::vector<std::unique_ptr<IAction>> actions;
    bool stopOnMatch = true;

    [[nodiscard]] bool matches(const std::filesystem::path& path, FileChangeFlags event) const {
        // 1. Bitmask check (instant reject)
        if ((triggerEvents & event) == FileChangeFlags::None) {
            return false;
        }

        // 2. Pattern check
        const std::string filename = path.filename().string();
        for (const auto& reg : compiledPatterns) {
            if (std::regex_match(filename, reg)) {
                return true;
            }
        }
        return false;
    }
};
#endif //RULE_H
