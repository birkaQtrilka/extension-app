//
// Created by Birka on 22/10/2026.
//

#include "ActionFactory.h"

void ActionFactory::registerAction(std::string name, Creator creator) {
    m_creators.insert_or_assign(std::move(name), std::move(creator));
}

ActionFactory::Result ActionFactory::create(const std::string &name, const nlohmann::json &json) const {
    const auto it = m_creators.find(name);
    if (it == m_creators.end()) return std::unexpected("Unknown action type: " + name);
    return it->second(json);
}
