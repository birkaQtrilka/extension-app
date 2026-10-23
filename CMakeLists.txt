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
configure_file(
        "${CMAKE_SOURCE_DIR}/.env"
        "${CMAKE_BINARY_DIR}/.env"
        COPYONLY
)
include(FetchContent)
FetchContent_Declare(
        curl
        URL https://curl.se/download/curl-8.22.0.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

block()
    set(BUILD_CURL_EXE OFF)
    set(BUILD_EXAMPLES OFF)
    set(BUILD_LIBCURL_DOCS OFF)
    set(BUILD_MISC_DOCS OFF)
    set(BUILD_TESTING OFF)

    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_STATIC_LIBS ON)

    # Disable optional features whose dependencies aren't installed.
    set(CURL_USE_LIBPSL OFF)

    # Use Windows' native TLS implementation.
    if(WIN32)
        set(CURL_USE_SCHANNEL ON)
    endif()

    FetchContent_MakeAvailable(curl)
endblock()

# Catch2 v3 from GitHub
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
        src/actions/implementations/MoveToAction.h
        src/core/Rule.h
        src/core/EventProcessor.cpp
        src/core/EventProcessor.h
        src/core/FileWatcherApp.h
        src/core/SettingsParser.h
        src/core/SettingsParser.cpp
        src/actions/implementations/RemovePatternAction.h
        src/actions/implementations/SendToTelegramAction.h
        src/core/TelegramClient.cpp
        src/core/TelegramClient.h
        src/core/Helpers/DotEnv.cpp
        src/core/Helpers/DotEnv.h
        src/actions/ActionFactory.cpp
        src/actions/ActionFactory.h
        src/actions/ActionRegistry.h
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
target_link_libraries(extension_app PRIVATE
        nlohmann_json::nlohmann_json
        CURL::libcurl
)

# 4. Enable test discovery in your IDE
include(CTest)
include(Catch)
catch_discover_tests(unit_tests)