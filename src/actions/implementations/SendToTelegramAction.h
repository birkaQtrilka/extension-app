#ifndef SENDTOTELEGRAMACTION_H
#define SENDTOTELEGRAMACTION_H

#include "core/IAction.h"
#include "core/TelegramClient.h"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

enum class SendType {
    AsFile,
    AsText
};

class SendToTelegramAction final : public IAction {
    static constexpr std::size_t kMaxMessageLength = 4096;     // Telegram limit per message
    static constexpr std::uintmax_t kMaxTextBytes = 256 * 1024; // refuse to spam huge files
    std::shared_ptr<const IMessageClient> m_client;
    SendType m_type;

    static ActionResult fail(const std::filesystem::path& path, std::string message) {
        return std::unexpected(ActionError{
            "SendToTelegram",
            path,
            std::make_error_code(std::errc::io_error),
            std::move(message)
        });
    }

    static std::expected<std::string, std::string>
    readTextFile(const std::filesystem::path& path) {
        std::error_code ec;
        const auto size = std::filesystem::file_size(path, ec);
        if (ec) {
            return std::unexpected("Cannot get file size: " + ec.message());
        }
        if (size == 0) {
            return std::unexpected("File is empty");
        }
        if (size > kMaxTextBytes) {
            return std::unexpected("File is too large to send as text");
        }

        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return std::unexpected("Cannot open file");
        }

        std::string content(size, '\0');
        file.read(content.data(), static_cast<std::streamsize>(size));
        if (!file) {
            return std::unexpected("Failed while reading file");
        }
        return content;
    }

    [[nodiscard]] ActionResult sendAsText(const std::filesystem::path& path) const {
        const auto text = readTextFile(path);

        if (!text) {
            return fail(path, text.error());
        }

        const std::string_view view{*text};
        for (std::size_t offset = 0; offset < view.size(); offset += kMaxMessageLength) {
            const auto piece = view.substr(offset, kMaxMessageLength);
            if (const auto sent = m_client->sendMessage(piece); !sent) {
                return fail(path, sent.error());
            }
        }
        return ActionSuccess{FileState::Preserved};
    }

    [[nodiscard]] ActionResult sendAsFile(const std::filesystem::path& path) const {
        if (const auto sent = m_client->sendDocument(path); !sent) {
            return fail(path, sent.error());
        }
        return ActionSuccess{FileState::Preserved};
    }

public:
    SendToTelegramAction(std::shared_ptr<const IMessageClient> client, const SendType type)
        : m_client(std::move(client)), m_type(type) {}


    ActionResult execute(std::filesystem::path& currentPath) const override {
        switch (m_type) {
            case SendType::AsFile: return sendAsFile(currentPath);
            case SendType::AsText: return sendAsText(currentPath);
        }
        std::unreachable();
    }
};

#endif // SENDTOTELEGRAMACTION_H