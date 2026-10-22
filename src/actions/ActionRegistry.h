//
// Created by Birka on 22/10/2026.
//

#ifndef ACTIONREGISTRY_H
#define ACTIONREGISTRY_H
#include <expected>
#include <nlohmann/json.hpp>

#include "ActionFactory.h"
#include "implementations/MoveToAction.h"
#include "implementations/RemovePatternAction.h"
#include "implementations/SendToTelegramAction.h"

struct ActionRegistry {
private:
    static std::expected<SendType, std::string> parseSendType(std::string_view value) {
        if (value == "file") return SendType::AsFile;
        if (value == "txt")  return SendType::AsText;
        return std::unexpected("Unknown sendMethod: " + std::string(value));
    }
public:
    static void registerBuiltInActions(ActionFactory& factory, std::shared_ptr<const TelegramClient> telegram) {
        factory.registerAction("sendToTelegram",
        [telegram](const nlohmann::json& json) -> ActionFactory::Result {
            if (!telegram) {
                return std::unexpected(
                    "sendToTelegram needs TELEGRAM_TOKEN and TELEGRAM_CHAT_ID in .env");
            }
            const std::string sendMethod = json.value("sendMethod", "");
            const auto type = parseSendType(json.value("sendMethod", "file"));
            if (!type) {
                return std::unexpected(type.error());
            }
            return std::make_unique<SendToTelegramAction>(telegram, *type);
        });

        factory.registerAction("removePattern",
            [](const nlohmann::json& json) -> ActionFactory::Result {
                std::string patternStr = json.value("pattern", "");
                if (patternStr.empty()) {
                    return std::unexpected("Missing 'pattern' property in removePattern action");
                }
                // Compile regex without '^', '$', or wildcards so it only matches the substring
                return std::make_unique<RemovePatternAction>(std::regex(patternStr));
            }
        );

        factory.registerAction("moveTo",
            [](const nlohmann::json& json) -> ActionFactory::Result {
                std::string destination = json.value("destination", "");
                if (destination.empty()) {
                    return std::unexpected("No destination specified");
                }
                return std::make_unique<MoveToAction>(destination);
            }
        );
    }
};

#endif //ACTIONREGISTRY_H
