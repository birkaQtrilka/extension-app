//
// Created by Birka on 15/10/2026.
//

#ifndef IACTION_H
#define IACTION_H
#include <expected>
#include <filesystem>

#include "ActionError.h"


class IAction {
public:
    virtual ~IAction() = default;
    // Returns true if the file still exists at 'currentPath' after execution,
    // false if it was moved/deleted (which affects subsequent chained actions).
    virtual ActionResult execute(const std::filesystem::path& currentPath) = 0;
};



#endif //IACTION_H
