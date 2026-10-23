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

# 1. libcurl
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

    set(CURL_USE_LIBPSL OFF)

    if(WIN32)
        set(CURL_USE_SCHANNEL ON)
    endif()

    FetchContent_MakeAvailable(curl)
endblock()

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.3
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

add_library(app_core STATIC
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
        src/core/IMessageClient.h
)

target_include_directories(app_core PUBLIC src)
target_link_libraries(app_core PUBLIC
        nlohmann_json::nlohmann_json
        CURL::libcurl
)

# Main Application
add_executable(extension_app src/main.cpp)
target_link_libraries(extension_app PRIVATE app_core)

# Unit Tests
add_executable(unit_tests
        tests/SettleDetectorTest.cpp
        tests/RuleEngineTest.cpp
        tests/SettingsParserTest.cpp
        tests/ActionsTest.cpp
        tests/SendToTelegramActionTest.cpp
)

target_include_directories(unit_tests PRIVATE tests)
target_link_libraries(unit_tests PRIVATE
        app_core
        Catch2::Catch2WithMain
)

# Test Discovery
include(CTest)
include(Catch)
catch_discover_tests(unit_tests)