#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>

#include "core/IFileProbe.h"
#include "core/SettleDetector.h"

// A fake probe that considers all paths to be stable regular files
class AlwaysStableProbe final : public IFileProbe {
public:
    [[nodiscard]] bool isRegularFile(const std::filesystem::path&) const override { return true; }
    [[nodiscard]] bool isStable(const std::filesystem::path&) const override { return true; }
};

class ChangeableProbe final : public IFileProbe {
public:
    void setStable(const bool stable) { m_stable = stable; }
    [[nodiscard]] bool isRegularFile(const std::filesystem::path&) const override { return true; }
    [[nodiscard]] bool isStable(const std::filesystem::path&) const override { return m_stable; }
private:
    bool m_stable = true;
};

TEST_CASE("Modified refreshes the quiet period") {
    using namespace std::chrono_literals;

    AlwaysStableProbe probe;
    SettleDetector detector(500ms, probe);
    const auto t0 = std::chrono::steady_clock::now();

    detector.onEvent(Created{"a.pdf"}, t0);
    detector.onEvent(Modified{"a.pdf"}, t0 + 400ms);

    // At 600ms: only 200ms has elapsed since the last Modified -> not ready yet
    REQUIRE(detector.collectReady(t0 + 600ms).empty());

    // At 900ms: 500ms quiet period has elapsed since t0 + 400ms -> ready!
    auto ready = detector.collectReady(t0 + 900ms);
    REQUIRE(ready.size() == 1);
    REQUIRE(ready[0].path == "a.pdf");
}

TEST_CASE("Unstable refreshes the quiet period") {
    using namespace std::chrono_literals;

    ChangeableProbe probe;
    SettleDetector detector(500ms, probe);
    const auto t0 = std::chrono::steady_clock::now();

    detector.onEvent(Created{"a.pdf"}, t0);
    probe.setStable(false);
    // should be empty and reset the timer to be at t0 + n
    REQUIRE(detector.collectReady(t0 + 600ms).empty());
    probe.setStable(true);

    REQUIRE(detector.collectReady(t0 + 900ms).empty());

    // At 900ms: 500ms quiet period has elapsed since t0 + 400ms -> ready!
    auto ready = detector.collectReady(t0 + 1100ms);
    REQUIRE(ready.size() == 1);
    REQUIRE(ready[0].path == "a.pdf");
}

TEST_CASE("Create and delete returns nothing") {
    using namespace std::chrono_literals;
    AlwaysStableProbe probe;
    SettleDetector detector(500ms, probe);
    const auto t0 = std::chrono::steady_clock::now();

    detector.onEvent(Created{"a.pdf"}, t0);
    detector.onEvent(Removed{"a.pdf"}, t0 + 10ms);
    detector.onEvent(Created{"b.pdf"}, t0);

    REQUIRE(detector.collectReady(t0 + 600ms).size() == 1);
}

TEST_CASE("Overflow does not lose pending files") {
    using namespace std::chrono_literals;
    AlwaysStableProbe probe;
    SettleDetector detector(500ms, probe);
    const auto t0 = std::chrono::steady_clock::now();

    detector.onEvent(Created{"a.pdf"}, t0);
    detector.onEvent(Created{"b.pdf"}, t0);
    detector.onEvent(Overflow{}, t0 + 10ms);

    REQUIRE(detector.collectReady(t0 + 600ms).size() == 2);
}