//
// Created by Birka on 18/10/2026.
//

#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <expected>
#include <vector>
#include <filesystem>
#include "Rule.h"

class ConfigParser {
public:
    explicit ConfigParser(std::filesystem::path file) : m_file(std::move(file)) {};
    std::expected<std::vector<Rule>, std::string> parse() const;
private:
    std::filesystem::path m_file;
};



#endif //CONFIGPARSER_H
