//
// Created by Birka on 15/10/2026.
//

#ifndef ACTIONERROR_H
#define ACTIONERROR_H
#include <expected>
#include <filesystem>
#include <system_error>


struct ActionError {
    std::string actionType;
    std::filesystem::path targetPath;
    std::error_code ec;
    std::string message;
};

enum class FileState {
    Preserved, // File is still at currentPath (e.g. after copyTo)
    Moved,     // File was moved to a new path (currentPath is updated)
    Deleted    // File no longer exists
};

struct ActionSuccess {
    FileState state;
};

using ActionResult = std::expected<ActionSuccess, ActionError>;

#endif //ACTIONERROR_H
