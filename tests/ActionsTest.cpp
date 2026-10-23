#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <regex>

#include "actions/implementations/RemovePatternAction.h"
#include "actions/implementations/MoveToAction.h"

namespace fs = std::filesystem;

struct TempDirectoryFixture {
    fs::path tempDir;

    TempDirectoryFixture() {
        tempDir = fs::temp_directory_path() / ("test_env_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        fs::create_directories(tempDir);
    }

    ~TempDirectoryFixture() {
        std::error_code ec;
        fs::remove_all(tempDir, ec);
    }

    fs::path createFile(const std::string& filename, const std::string& content = "dummy") {
        fs::path p = tempDir / filename;
        std::ofstream out(p);
        out << content;
        return p;
    }
};

TEST_CASE_METHOD(TempDirectoryFixture, "RemovePatternAction renames file correctly on match") {
    auto filePath = createFile("invoice_COPY_2026.pdf");

    // Remove "_COPY"
    RemovePatternAction action(std::regex("_COPY"));
    auto result = action.execute(filePath);

    REQUIRE(result.has_value());
    CHECK(result->state == FileState::Preserved);
    CHECK(filePath.filename() == "invoice_2026.pdf");
    CHECK(fs::exists(filePath));
}

TEST_CASE_METHOD(TempDirectoryFixture, "MoveToAction relocates file to destination directory") {
    auto filePath = createFile("data.csv");
    fs::path destDir = tempDir / "archive";

    MoveToAction action(destDir);
    auto result = action.execute(filePath);

    REQUIRE(result.has_value());
    CHECK(result->state == FileState::Moved);
    CHECK(filePath == destDir / "data.csv");
    CHECK(fs::exists(filePath));
    CHECK_FALSE(fs::exists(tempDir / "data.csv"));
}