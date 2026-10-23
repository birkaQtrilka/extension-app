#ifndef IMESSAGECLIENT_H
#define IMESSAGECLIENT_H
#include <expected>
#include <string>
#include <filesystem>

class IMessageClient {
public:
    virtual ~IMessageClient() = default;
    [[nodiscard]] virtual std::expected<void, std::string> sendMessage(std::string_view text) const = 0;
    [[nodiscard]] virtual std::expected<void, std::string> sendDocument(const std::filesystem::path& file) const = 0;
};
#endif //IMESSAGECLIENT_H
