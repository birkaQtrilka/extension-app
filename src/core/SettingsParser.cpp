//
// Created by Birka on 19/10/2026.
//
#include "SettingsParser.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

std::expected<AppConfig, std::string> SettingsParser::parse() const {
    std::ifstream configFile(m_path);
    if (!configFile.is_open()) {
        return std::unexpected(
            std::string("Config file could not be opened: ") + m_path.string()
        );
    }

    nlohmann::json jsonConfig;
    try {
        jsonConfig = nlohmann::json::parse(configFile);
    } catch (const nlohmann::json::parse_error& e) {
        return std::unexpected(std::string("JSON Parse Error: ") + e.what());
    }

    if (!jsonConfig.is_object()) {
        return std::unexpected(
            std::string("Root JSON element must be an object")
        );
    }

    if (!jsonConfig.contains("watched_folder")) {
        return std::unexpected(
            std::string("No watch folder specified")
        );
    }

    if (!jsonConfig["watched_folder"].is_string()) {
        return std::unexpected(
            std::string("'watched_folder' must be a string")
        );
    }

    const std::string watch = jsonConfig["watched_folder"].get<std::string>();
    const std::string rules = jsonConfig.contains("rules_path")
        ? jsonConfig["rules_path"].get<std::string>()
        : "rules.json";

    if (watch.empty()) {
        return std::unexpected(
            std::string("'watched_folder' cannot be empty")
        );
    }

    AppConfig config;
    config.watchPath = std::filesystem::path(watch);
    config.configPath = rules;

    if (jsonConfig.contains("debounce_time_ms")) {
        if (!jsonConfig["debounce_time_ms"].is_number_unsigned() &&
            !jsonConfig["debounce_time_ms"].is_number_integer()) {
            return std::unexpected(
                std::string("'debounce_time_ms' must be an integer")
            );
            }

        const auto debounce =
            jsonConfig["debounce_time_ms"].get<std::int64_t>();

        if (debounce < 0) {
            return std::unexpected(
                std::string("'debounce_time_ms' cannot be negative")
            );
        }

        config.quietPeriod = std::chrono::milliseconds(debounce);
    }

    return config;
}
