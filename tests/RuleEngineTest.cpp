#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <regex>
#include <vector>

#include "core/IAction.h"
#include "core/RuleEngine.h"

// Mock action that records invocations and can simulate deletion or failure
class MockAction final : public IAction {
public:
    explicit MockAction(FileState returnState = FileState::Preserved, bool shouldFail = false)
        : m_returnState(returnState), m_shouldFail(shouldFail) {}

    ActionResult execute(std::filesystem::path& path) const override {
        callCount++;
        lastReceivedPath = path;

        if (m_shouldFail) {
            return std::unexpected(ActionError{
                .actionType = "MockAction",
                .targetPath = path,
                .ec = std::make_error_code(std::errc::permission_denied),
                .message = "Simulated failure"
            });
        }
        return ActionSuccess{ .state = m_returnState };
    }

    mutable int callCount = 0;
    mutable std::filesystem::path lastReceivedPath;
private:
    FileState m_returnState;
    bool m_shouldFail;
};

TEST_CASE("Rule matching correctly checks bitmask events") {
    Rule rule;
    rule.triggerEvents = FileChangeFlags::Created | FileChangeFlags::Modified;
    rule.compiledPatterns.emplace_back(R"(^.*\.txt$)", std::regex_constants::icase);

    CHECK(rule.matches("doc.txt", FileChangeFlags::Created));
    CHECK(rule.matches("doc.txt", FileChangeFlags::Modified));
    CHECK_FALSE(rule.matches("doc.txt", FileChangeFlags::Renamed));
    CHECK_FALSE(rule.matches("doc.pdf", FileChangeFlags::Created));
}

TEST_CASE("RuleEngine executes action chain when rule matches") {
    auto action1 = std::make_unique<MockAction>();
    auto action2 = std::make_unique<MockAction>();
    auto* ptr1 = action1.get();
    auto* ptr2 = action2.get();

    Rule rule;
    rule.triggerEvents = FileChangeFlags::Created;
    rule.compiledPatterns.emplace_back(R"(^.*\.pdf$)");
    rule.actions.push_back(std::move(action1));
    rule.actions.push_back(std::move(action2));

    std::vector<Rule> rules;
    rules.push_back(std::move(rule));
    RuleEngine engine(std::move(rules));

    FileReady readyFile{
        .path = "invoice.pdf",
        .events = FileChangeFlags::Created
    };

    auto result = engine.executeActions(readyFile);
    REQUIRE(result.has_value());
    CHECK(ptr1->callCount == 1);
    CHECK(ptr2->callCount == 1);
}

TEST_CASE("RuleEngine stops action pipeline when file is marked Deleted") {
    auto deleteAction = std::make_unique<MockAction>(FileState::Deleted);
    auto downstreamAction = std::make_unique<MockAction>(FileState::Preserved);
    auto* ptrDownstream = downstreamAction.get();

    Rule rule;
    rule.triggerEvents = FileChangeFlags::Created;
    rule.compiledPatterns.emplace_back(R"(.*)");
    rule.actions.push_back(std::move(deleteAction));
    rule.actions.push_back(std::move(downstreamAction));

    std::vector<Rule> rules;
    rules.push_back(std::move(rule));
    RuleEngine engine(std::move(rules));

    FileReady readyFile{ .path = "temp.tmp", .events = FileChangeFlags::Created };
    auto result = engine.executeActions(readyFile);

    REQUIRE(result.has_value());
    CHECK(ptrDownstream->callCount == 0);
}

TEST_CASE("RuleEngine stops at first rule when stopOnMatch is true") {
    auto rule1Action = std::make_unique<MockAction>();
    auto rule2Action = std::make_unique<MockAction>();
    auto* p1 = rule1Action.get();
    auto* p2 = rule2Action.get();

    Rule rule1;
    rule1.stopOnMatch = true;
    rule1.triggerEvents = FileChangeFlags::Created;
    rule1.compiledPatterns.emplace_back(R"(.*)");
    rule1.actions.push_back(std::move(rule1Action));

    Rule rule2;
    rule2.stopOnMatch = true;
    rule2.triggerEvents = FileChangeFlags::Created;
    rule2.compiledPatterns.emplace_back(R"(.*)");
    rule2.actions.push_back(std::move(rule2Action));

    std::vector<Rule> rules;
    rules.push_back(std::move(rule1));
    rules.push_back(std::move(rule2));
    RuleEngine engine(std::move(rules));

    FileReady readyFile{ .path = "test.txt", .events = FileChangeFlags::Created };
    REQUIRE(engine.executeActions(readyFile).has_value());

    CHECK(p1->callCount == 1);
    CHECK(p2->callCount == 0); // Second rule must not execute
}