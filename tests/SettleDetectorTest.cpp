#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <unordered_set>

#include "core/IFileProbe.h"
#include "core/SettleDetector.h"

class AlwaysStableProbe final : public IFileProbe {
public:
    [[nodiscard]] bool isRegularFile(const std::filesystem::path&) const override { return true; }
    [[nodiscard]] bool isStable(const std::filesystem::path&) const override { return true; }
};

class PathSelectiveProbe final : public IFileProbe {
public:
    void markUnstable(const std::filesystem::path& p) { m_unstable.insert(p); }
    void markStable(const std::filesystem::path& p)   { m_unstable.erase(p); }

    [[nodiscard]] bool isRegularFile(const std::filesystem::path&) const override { return true; }
    [[nodiscard]] bool isStable(const std::filesystem::path& p) const override {
        return !m_unstable.contains(p);
    }
private:
    std::unordered_set<std::filesystem::path> m_unstable;
};

TEST_CASE("Tracks multiple files with independent deadlines") {
    using namespace std::chrono_literals;

    AlwaysStableProbe probe;
    SettleDetector detector(500ms, probe);
    const auto t0 = std::chrono::steady_clock::now();

    detector.onEvent(FileEvent::created("first.txt"), t0);
    detector.onEvent(FileEvent::created("second.txt"), t0 + 200ms);

    auto ready1 = detector.collectReady(t0 + 550ms);
    REQUIRE(ready1.size() == 1);
    CHECK(ready1[0].path == "first.txt");

    REQUIRE(detector.collectReady(t0 + 600ms).empty());

    auto ready2 = detector.collectReady(t0 + 750ms);
    REQUIRE(ready2.size() == 1);
    CHECK(ready2[0].path == "second.txt");
}

TEST_CASE("Event flags aggregate across multiple events") {
    using namespace std::chrono_literals;

    AlwaysStableProbe probe;
    SettleDetector detector(500ms, probe);
    const auto t0 = std::chrono::steady_clock::now();

    detector.onEvent(FileEvent::created("report.docx"), t0);
    detector.onEvent(FileEvent::modified("report.docx"), t0 + 100ms);

    auto ready = detector.collectReady(t0 + 650ms);
    REQUIRE(ready.size() == 1);
    CHECK(ready[0].path == "report.docx");
    CHECK(hasFlag(ready[0].events, FileChangeFlags::Created));
    CHECK(hasFlag(ready[0].events, FileChangeFlags::Modified));
}

TEST_CASE("pruneMissing removes files no longer on disk") {
    using namespace std::chrono_literals;

    AlwaysStableProbe probe;
    SettleDetector detector(500ms, probe);
    const auto t0 = std::chrono::steady_clock::now();

    detector.onEvent(FileEvent::created("deleted_external.pdf"), t0);
    detector.onEvent(FileEvent::created("kept.pdf"), t0);

    // Directory rescan only discovers kept.pdf
    std::unordered_set<std::filesystem::path> diskFiles = { "kept.pdf" };
    detector.pruneMissing(diskFiles);

    auto ready = detector.collectReady(t0 + 600ms);
    REQUIRE(ready.size() == 1);
    CHECK(ready[0].path == "kept.pdf");
}

TEST_CASE("Renamed event migrates tracked state to new path") {
    using namespace std::chrono_literals;

    AlwaysStableProbe probe;
    SettleDetector detector(500ms, probe);
    const auto t0 = std::chrono::steady_clock::now();

    detector.onEvent(FileEvent::created("old_name.txt"), t0);
    detector.onEvent(FileEvent::renamed("old_name.txt", "new_name.txt"), t0 + 100ms);

    auto ready = detector.collectReady(t0 + 700ms);
    REQUIRE(ready.size() == 1);
    CHECK(ready[0].path == "new_name.txt");
    CHECK(hasFlag(ready[0].events, FileChangeFlags::Renamed));
}