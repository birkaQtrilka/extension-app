//
// Created by Birka on 15/10/2026.
//

#ifndef IACTION_H
#define IACTION_H
#include <expected>
#include <string_view>

#include "ActionError.h"
#include "FileEvent.h"


class IAction {
public:
    virtual ~IAction() = default;
    virtual std::string_view name() const = 0;
    virtual std::expected<void, ActionError> execute(const FileEvent& event) = 0;
};



#endif //IACTION_H
