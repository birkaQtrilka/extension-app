#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include "core/SettingsParser.h"

struct TempSettingsFile {
    std::filesystem::path path;
    explicit TempSettingsFile(std::string_view content) {
        path = std::filesystem::temp_directory_path() / "test_settings.json";
        std::ofstream out(path);
        out << content;
    }
    ~TempSettingsFile() {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
};

TEST_CASE("SettingsParser rejects missing watched_folder") {
    TempSettingsFile file(R"({ "debounce_time_ms": 300 })");
    SettingsParser parser(file.path);
    auto res = parser.parse();

    REQUIRE_FALSE(res.has_value());
    CHECK(res.error().find("No watch folder specified") != std::string::npos);
}

TEST_CASE("SettingsParser rejects negative debounce time") {
    TempSettingsFile file(R"({ "watched_folder": "C:/Watch", "debounce_time_ms": -100 })");
    SettingsParser parser(file.path);
    auto res = parser.parse();

    REQUIRE_FALSE(res.has_value());
    CHECK(res.error().find("cannot be negative") != std::string::npos);
}

TEST_CASE("SettingsParser successfully parses valid config") {
    using namespace std::chrono_literals;

    TempSettingsFile file(R"({
        "watched_folder": "/var/watch",
        "rules_path": "/var/rules.json",
        "debounce_time_ms": 750
    })");

    SettingsParser parser(file.path);
    auto res = parser.parse();

    REQUIRE(res.has_value());
    CHECK(res->watchPath == std::filesystem::path("/var/watch"));
    CHECK(res->quietPeriod == 750ms);
}