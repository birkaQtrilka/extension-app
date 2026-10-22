#ifndef TELEGRAMCLIENT_H
#define TELEGRAMCLIENT_H

#include <expected>
#include <string>
#include <string_view>
#include <filesystem>
#include <system_error>
#include <bits/fs_path.h>

class TelegramClient {
public:
    TelegramClient(std::string botToken, std::string chatId);
    ~TelegramClient();

    TelegramClient(const TelegramClient&) = delete;
    TelegramClient& operator=(const TelegramClient&) = delete;

    [[nodiscard]]
    std::expected<void, std::string>
    sendMessage(std::string_view text) const;

    [[nodiscard]] std::expected<void, std::string>
    sendDocument(const std::filesystem::path& file) const;

private:
    std::string m_botToken;
    std::string m_chatId;
};

#endif // TELEGRAMCLIENT_H