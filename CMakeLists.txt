cmake_minimum_required(VERSION 3.30)
project(extension_app)
set(CMAKE_CXX_STANDARD 26)

configure_file(
        "${CMAKE_SOURCE_DIR}/rules.json"
        "${CMAKE_BINARY_DIR}/rules.json"
        COPYONLY
)
configure_file(
        "${CMAKE_SOURCE_DIR}/settings.json"
        "${CMAKE_BINARY_DIR}/settings.json"
        COPYONLY
)


# Catch2 v3 from GitHub
include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.3 # Or latest v3 release
)
FetchContent_MakeAvailable(Catch2)
# json from GitHub
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)


add_executable(extension_app src/main.cpp
        src/core/IAction.h
        src/core/IFolderWatcher.h
        src/core/FileEvent.h
        src/core/ActionError.h
        src/platform/windows/WindowsWatcher.cpp
        src/platform/windows/WindowsWatcher.h
        src/platform/windows/HandleCloser.h
        src/core/ThreadSafeQueue.h
        src/core/SettleDetector.h
        src/core/SettleDetector.cpp
        src/core/IFileProbe.h
        src/platform/windows/WindowsFileProbe.h
        src/core/RuleEngine.cpp
        src/core/RuleEngine.h
        src/core/ConfigParser.cpp
        src/core/ConfigParser.h
        src/actions/MoveToAction.h
        src/core/EventKind.h
        src/core/Rule.h
        src/core/EventProcessor.cpp
        src/core/EventProcessor.h
        src/core/FileWatcherApp.h
        src/core/SettingsParser.h
        src/core/SettingsParser.cpp
        src/actions/RemovePatternAction.h
)
target_include_directories(extension_app PRIVATE src)

# 2. Add your test executable
add_executable(unit_tests
        tests/SettleDetectorTest.cpp
        src/core/SettleDetector.cpp
        # Add any other source files needed by the test
)
target_include_directories(unit_tests PRIVATE src tests)
# 3. Link Catch2 (Catch2WithMain provides int main() automatically!)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain)
target_link_libraries(extension_app PRIVATE nlohmann_json::nlohmann_json)

# 4. Enable test discovery in your IDE
include(CTest)
include(Catch)
catch_discover_tests(unit_tests)