//
// Created by Birka on 19/10/2026.
//

#ifndef SETTINGSPARSER_H
#define SETTINGSPARSER_H
#include <filesystem>
#include <string>
#include <expected>

#include "FileWatcherApp.h"

class SettingsParser {
    public:
    explicit SettingsParser(std::filesystem::path path) : m_path(std::move(path)) {};
    ~SettingsParser() = default;

    std::expected<AppConfig, std::string> parse() const;
    private:
    std::filesystem::path m_path;
};
#endif //SETTINGSPARSER_H
