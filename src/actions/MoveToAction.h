//
// Created by Birka on 18/10/2026.
//

#ifndef MOVEACTION_H
#define MOVEACTION_H
#include "core/IAction.h"

class MoveToAction final : public IAction {
    using path = std::filesystem::path;
    path destinationDir_;
public:
    explicit MoveToAction(path dest) : destinationDir_(std::move(dest)) {}

    ActionResult execute(path& currentPath) override {
        std::error_code ec;
        std::filesystem::create_directories(destinationDir_, ec);
        if (ec) {
            return std::unexpected(ActionError{
                .actionType = "moveTo",
                .targetPath = destinationDir_,
                .ec = ec,
                .message = "Failed to create target directory"
            });
        }

        const path target = destinationDir_ / currentPath.filename();
        std::filesystem::rename(currentPath, target, ec);
        if (ec) {
            return std::unexpected(ActionError{
                .actionType = "moveTo",
                .targetPath = target,
                .ec = ec,
                .message = "Failed to move file"
            });
        }
        currentPath = target;
        return ActionSuccess{ .state = FileState::Moved }; // File no longer exists at original location
    }
};
#endif //MOVEACTION_H
