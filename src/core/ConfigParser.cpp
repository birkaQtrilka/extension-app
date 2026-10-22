//
// Created by Birka on 18/10/2026.
//

#include "ConfigParser.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <expected>
#include <algorithm>
#include <cctype>

#include "TelegramClient.h"
#include "actions/RemovePatternAction.h"

namespace {
    // Helper: Converts glob-style wildcard string (e.g., "*.cv") to a regex string (e.g., "^.*\.cv$")
    std::string globToRegexString(const std::string& glob) {
        std::string regexStr = "^";
        for (const char c : glob) {
            switch (c) {
                case '*': regexStr += ".*"; break;
                case '?': regexStr += "."; break;
                case '.': regexStr += "\\."; break;
                case '\\': regexStr += "\\\\"; break;
                // Escape regex special characters
                case '[': case ']': case '(': case ')':
                case '{': case '}': case '+': case '|': case '^': case '$':
                    regexStr += '\\';
                    regexStr += c;
                    break;
                default:
                    regexStr += c;
                    break;
            }
        }
        regexStr += "$";
        return regexStr;
    }

    // Helper: Parse event string dynamically to EventKind
    EventKind stringToEventKind(const std::string& eventStr) {
        std::string lowerStr = eventStr;
        std::ranges::transform(lowerStr, lowerStr.begin(),
                               [](const unsigned char c){ return std::tolower(c); });

        if (lowerStr == "created") return EventKind::Created;
        if (lowerStr == "renamed") return EventKind::Renamed;
        if (lowerStr == "init") return EventKind::Init;

        return EventKind::None;
    }
} // anonymous namespace


std::expected<std::vector<Rule>, std::string> ConfigParser::parse() const {
    std::ifstream configFile(m_file);
    if (!configFile.is_open()) {
        return std::unexpected(std::string("Config file could not be opened: ") + m_file.string());
    }

    nlohmann::json jsonConfig;
    try {
        jsonConfig = nlohmann::json::parse(configFile);
    } catch (const nlohmann::json::parse_error& e) {
        return std::unexpected(std::string("JSON Parse Error: ") + e.what());
    }

    if (!jsonConfig.is_array()) {
        return std::unexpected(std::string("Root JSON element must be an array of rules."));
    }

    std::vector<Rule> rules;

    try {
        for (const auto& jsonRule : jsonConfig) {
            Rule rule;

            rule.stopOnMatch = jsonRule.value("stopOnMatch", true);

            // Parse match conditions
            if (jsonRule.contains("match") && jsonRule["match"].is_object()) {
                const auto& matchObj = jsonRule["match"];

                // Parse patterns
                if (matchObj.contains("patterns") && matchObj["patterns"].is_array()) {
                    for (const auto& pat : matchObj["patterns"]) {
                        std::string globPattern = pat.get<std::string>();
                        std::string regexStr = globToRegexString(globPattern);
                        rule.compiledPatterns.emplace_back(regexStr);
                    }
                }

                // Parse events
                if (matchObj.contains("events") && matchObj["events"].is_array()) {
                    for (const auto& ev : matchObj["events"]) {
                        rule.triggerEvents = rule.triggerEvents | stringToEventKind(ev.get<std::string>());
                    }
                }
            } else {
                return std::unexpected(std::string("A rule is missing the 'match' object."));
            }

            // Parse actions
            if (jsonRule.contains("actions") && jsonRule["actions"].is_array()) {
                for (const auto& actionObj : jsonRule["actions"]) {
                    if (!actionObj.contains("type")) {
                        return std::unexpected(std::string("An action is missing the 'type' field."));
                    }

                    std::string actionType = actionObj["type"].get<std::string>();
                    auto expectedAction = m_actionFactory.create(actionType, actionObj);
                    if (!expectedAction) return std::unexpected(expectedAction.error());
                    rule.actions.push_back(std::move(expectedAction.value()));
                }
            } else {
                return std::unexpected(std::string("A rule is missing the 'actions' array."));
            }

            rules.push_back(std::move(rule));
        }
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(std::string("JSON structure error: ") + e.what());
    } catch (const std::regex_error& e) {
        return std::unexpected(std::string("Regex compilation error: ") + e.what());
    } catch (const std::exception& e) {
        return std::unexpected(std::string("General error during parsing: ") + e.what());
    }

    return rules;
}
