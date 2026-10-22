#include <iostream>
#include <chrono>
#include <nlohmann/json.hpp>

#include "actions/ActionFactory.h"
#include "actions/ActionRegistry.h"
#include "core/FileWatcherApp.h"
#include "core/SettingsParser.h"
#include "core/TelegramClient.h"
#include "core/Helpers/DotEnv.h"
#include "platform/windows/WindowsWatcher.h"
#include "platform/windows/WindowsFileProbe.h"


int main() {
    SettingsParser settings_parser("settings.json");
    auto expect = settings_parser.parse();
    if (!expect) {
        std::cerr << expect.error() << std::endl;
        return 1;
    }
    std::shared_ptr<const TelegramClient> telegram;

    auto env = DotEnv::load(".env");
    if (!env) {
        std::cerr << "Telegram disabled: " << env.error() << '\n';
    } else {
        const auto token  = env->get("TELEGRAM_TOKEN");
        const auto chatId = env->get("TELEGRAM_CHAT_ID");
        if (token && chatId) {
            telegram = std::make_shared<const TelegramClient>(*token, *chatId);
        } else {
            std::cerr << "Telegram disabled: TELEGRAM_TOKEN or TELEGRAM_CHAT_ID missing\n";
        }
    }

    ActionFactory factory;
    ActionRegistry::registerBuiltInActions(factory, telegram);

    AppConfig config = *expect;
    FileWatcherApp<WindowsWatcher, WindowsFileProbe> app(
        std::move(config),
        WindowsFileProbe{},
        std::move(factory)
    );
    if (!app.init()) {
        std::cerr << "Initialization failed. Exiting...\n";
        return 1;
    }

    app.start();
    std::cout << "Press Enter to stop...\n\n\n";

    std::cin.get();

    app.stop();

    return 0;
}