//
// Created by Birka on 19/10/2026.
//

#ifndef REMOVEPATTERNACTION_H
#define REMOVEPATTERNACTION_H

#include "core/IAction.h"
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

class RemovePatternAction final : public IAction {
    std::regex m_pattern;

public:
    explicit RemovePatternAction(std::regex pattern)
        : m_pattern(std::move(pattern)) {}

    ActionResult execute(std::filesystem::path &currentPath) const override {
        const std::string filename = currentPath.filename().string();

        if (std::regex_search(filename, m_pattern)) {
            // Replace the matched pattern with an empty string
            const std::string newFilename = std::regex_replace(filename, m_pattern, "");
            const std::filesystem::path newPath = currentPath.parent_path() / newFilename;
            std::error_code ec;
            std::filesystem::rename(currentPath, newPath, ec);
            if (ec) {
                return std::unexpected{ActionError{
                "RemovePattern",
                    currentPath,
                    ec,
                    "Could not rename file    "}
                };
            }
            std::cout << "Removed pattern and now is: " << newFilename << std::endl;
            // Update the referenced path to the new path
            currentPath = newPath;
        }

        return ActionSuccess {FileState::Preserved};
    }
};

#endif //REMOVEPATTERNACTION_H