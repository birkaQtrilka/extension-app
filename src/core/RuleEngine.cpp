//
// Created by Birka on 18/10/2026.
//

#include "RuleEngine.h"

std::expected<bool, ActionError> RuleEngine::executeActions(const FileReady &file) const {
    std::filesystem::path currentPath = file.path;

    for (const auto& rule : m_rules) {
        if (!rule.matches(currentPath, file.events)) continue;

        for (const auto& action : rule.actions) {
            auto result = action->execute(currentPath);

            if (!result) {
                return std::unexpected( result.error());
            }

            if (result->state == FileState::Deleted) {
                return {}; // No file left to perform actions on
            }
        }

        // If an error occurred or the file was deleted/moved, abort downstream rules
        if (rule.stopOnMatch) {
            return {};
        }
    }
    return {};
}
