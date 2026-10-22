//
// Created by Birka on 18/10/2026.
//

#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <expected>
#include <vector>
#include <string>
#include <filesystem>
#include "Rule.h"
#include "actions/ActionFactory.h"

class ConfigParser {
private:
    std::filesystem::path m_file;
    const ActionFactory& m_actionFactory;
public:
    explicit ConfigParser(std::filesystem::path file, const ActionFactory& action_factory)
    : m_file(std::move(file)), m_actionFactory(action_factory) {}
    [[nodiscard]] std::expected<std::vector<Rule>, std::string> parse() const;
};



#endif //CONFIGPARSER_H
