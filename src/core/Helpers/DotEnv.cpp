//
// Created by Birka on 22/10/2026.
//

#include "DotEnv.h"

#include <fstream>
std::optional<std::string> DotEnv::get(std::string_view key) const {
    const auto it = m_values.find(std::string(key));
    if (it == m_values.end() || it->second.empty()) {
        return std::nullopt;
    }
    return it->second;
}

std::expected<DotEnv, std::string> DotEnv::load(const std::filesystem::path &file) {
    std::ifstream in(file);
    if (!in) {
        return std::unexpected("Cannot open " + file.string());
    }

    DotEnv env;
    std::string line;
    while (std::getline(in, line)) {
        // 2. skip empty lines and lines starting with '#'
        // 3. split at the first '='
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key   = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));

        env.m_values.emplace(std::move(key), std::move(value));
    }
    return env;
}

std::string DotEnv::trim(std::string_view txt) {
    constexpr std::string_view whitespace = " \t\r\n";
    const auto start = txt.find_first_not_of(whitespace);
    if (start == std::string_view::npos) return {};
    txt = txt.substr(0, txt.find_last_not_of(whitespace) + 1);
    txt.remove_prefix(start);
    return std::string(txt);
}

// '   gfsdfs'
// 'gfsdfs   '
// '   gfsdfs    '
// 'gfs dfs'
// '     '
// 'gfsdfs'