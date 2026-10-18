//
// Created by Birka on 18/10/2026.
//

#include "ConfigParser.h"
#include <nlohmann/json.hpp>
#include <fstream>

std::expected<std::vector<Rule>, std::string> ConfigParser::parse() {
    std::ifstream configFile(m_file);
    if (!configFile.is_open()) return std::unexpected("Config file could not be opened");
    nlohmann::json::parse(configFile);
}
