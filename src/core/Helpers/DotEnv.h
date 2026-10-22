//
// Created by Birka on 22/10/2026.
//

#ifndef DOTENV_H
#define DOTENV_H
#include <expected>
#include <string>
#include <filesystem>
#include <optional>
#include <unordered_map>


class DotEnv {
    std::unordered_map<std::string, std::string> m_values;

public:
    [[nodiscard]] static std::expected<DotEnv, std::string>
    load(const std::filesystem::path& file);

    [[nodiscard]] std::optional<std::string>
    get(std::string_view key) const;

    [[nodiscard]] static std::string
    trim(std::string_view key);
};



#endif //DOTENV_H
