#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "actions/implementations/SendToTelegramAction.h"
#include "core/IMessageClient.h"

namespace fs = std::filesystem;

// Mock implementation of ITelegramClient
class MockTelegramClient final : public IMessageClient {
public:
    mutable std::vector<std::string> sentMessages;
    mutable std::vector<fs::path> sentDocuments;

    bool shouldFail = false;
    std::string failureMessage = "Network timeout";
    int failOnMessageIndex = -1; // Fail on the N-th sendMessage invocation

    [[nodiscard]] std::expected<void, std::string> sendMessage(std::string_view text) const override {
        if (shouldFail || (failOnMessageIndex >= 0 && static_cast<int>(sentMessages.size()) == failOnMessageIndex)) {
            return std::unexpected(failureMessage);
        }
        sentMessages.emplace_back(text);
        return {};
    }

    [[nodiscard]] std::expected<void, std::string> sendDocument(const fs::path& file) const override {
        if (shouldFail) {
            return std::unexpected(failureMessage);
        }
        sentDocuments.push_back(file);
        return {};
    }
};

// Fixture managing an isolated temporary folder for filesystem tests
struct TelegramActionFixture {
    fs::path tempDir;

    TelegramActionFixture() {
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        tempDir = fs::temp_directory_path() / ("tg_test_" + std::to_string(now));
        fs::create_directories(tempDir);
    }

    ~TelegramActionFixture() {
        std::error_code ec;
        fs::remove_all(tempDir, ec);
    }

    [[nodiscard]] fs::path createTestFile(const std::string& name, const std::string& content) const {
        fs::path p = tempDir / name;
        std::ofstream out(p, std::ios::binary);
        out << content;
        return p;
    }
};

TEST_CASE_METHOD(TelegramActionFixture, "SendToTelegramAction: send as document success") {
    auto mock = std::make_shared<MockTelegramClient>();
    SendToTelegramAction action(mock, SendType::AsFile);

    auto file = createTestFile("report.pdf", "PDF header and content");

    auto result = action.execute(file);

    REQUIRE(result.has_value());
    CHECK(result->state == FileState::Preserved);
    REQUIRE(mock->sentDocuments.size() == 1);
    CHECK(mock->sentDocuments[0] == file);
    CHECK(mock->sentMessages.empty());
}

TEST_CASE_METHOD(TelegramActionFixture, "SendToTelegramAction: send as document failure propagates error") {
    auto mock = std::make_shared<MockTelegramClient>();
    mock->shouldFail = true;
    mock->failureMessage = "Telegram API 401 Unauthorized";

    SendToTelegramAction action(mock, SendType::AsFile);
    auto file = createTestFile("secret.pdf", "data");

    auto result = action.execute(file);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().actionType == "SendToTelegram");
    CHECK(result.error().message.find("Telegram API 401 Unauthorized") != std::string::npos);
}

TEST_CASE_METHOD(TelegramActionFixture, "SendToTelegramAction: send text file under 4096 bytes in one call") {
    auto mock = std::make_shared<MockTelegramClient>();
    SendToTelegramAction action(mock, SendType::AsText);

    const std::string content = "Hello, this is a short log file line.";
    auto file = createTestFile("short.log", content);

    auto result = action.execute(file);

    REQUIRE(result.has_value());
    CHECK(result->state == FileState::Preserved);
    REQUIRE(mock->sentMessages.size() == 1);
    CHECK(mock->sentMessages[0] == content);
}

TEST_CASE_METHOD(TelegramActionFixture, "SendToTelegramAction: rejects empty files") {
    auto mock = std::make_shared<MockTelegramClient>();
    SendToTelegramAction action(mock, SendType::AsText);

    auto file = createTestFile("empty.txt", "");

    auto result = action.execute(file);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message.find("File is empty") != std::string::npos);
    CHECK(mock->sentMessages.empty());
}

TEST_CASE_METHOD(TelegramActionFixture, "SendToTelegramAction: rejects files larger than 256 KB") {
    auto mock = std::make_shared<MockTelegramClient>();
    SendToTelegramAction action(mock, SendType::AsText);

    // 256 KB = 262,144 bytes; generate 262,145 bytes
    const std::string hugeContent(256 * 1024 + 1, 'X');
    auto file = createTestFile("huge.txt", hugeContent);

    auto result = action.execute(file);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message.find("File is too large to send as text") != std::string::npos);
    CHECK(mock->sentMessages.empty());
}

TEST_CASE_METHOD(TelegramActionFixture, "SendToTelegramAction: chunks messages exceeding 4096 characters") {
    auto mock = std::make_shared<MockTelegramClient>();
    SendToTelegramAction action(mock, SendType::AsText);

    // 5000 characters: chunk 1 must be 4096 bytes, chunk 2 must be 904 bytes
    const std::string part1(4096, 'A');
    const std::string part2(904, 'B');
    const std::string fullContent = part1 + part2;

    auto file = createTestFile("long.txt", fullContent);

    auto result = action.execute(file);

    REQUIRE(result.has_value());
    REQUIRE(mock->sentMessages.size() == 2);
    CHECK(mock->sentMessages[0] == part1);
    CHECK(mock->sentMessages[1] == part2);
}

TEST_CASE_METHOD(TelegramActionFixture, "SendToTelegramAction: stops and returns error if a chunk fails mid-stream") {
    auto mock = std::make_shared<MockTelegramClient>();
    // Fail on the second chunk (index 1)
    mock->failOnMessageIndex = 1;
    mock->failureMessage = "Rate limit hit";

    SendToTelegramAction action(mock, SendType::AsText);

    const std::string fullContent(5000, 'Z');
    auto file = createTestFile("rate_limited.txt", fullContent);

    auto result = action.execute(file);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message.find("Rate limit hit") != std::string::npos);
    // Verified that only the first chunk succeeded before aborting
    CHECK(mock->sentMessages.size() == 1);
}