//
// Created by Birka on 22/10/2026.
//

#ifndef ACTIONFACTORY_H
#define ACTIONFACTORY_H
#include <expected>
#include <functional>
#include <memory>
#include <nlohmann/json_fwd.hpp>

#include "core/IAction.h"


class ActionFactory {
public:
    using Result  = std::expected<std::unique_ptr<IAction>, std::string>;
    using Creator = std::function<Result(const nlohmann::json&)>;

    void registerAction(std::string name, Creator creator);

    [[nodiscard]] Result create(const std::string& name,
                                const nlohmann::json& json) const;

private:
    std::unordered_map<std::string, Creator> m_creators;
};


#endif //ACTIONFACTORY_H
