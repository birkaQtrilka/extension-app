cmake_minimum_required(VERSION 3.30)
project(extension_app)
set(CMAKE_CXX_STANDARD 26)

# 1. Automatically download Catch2 v3 from GitHub
include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.3 # Or latest v3 release
)
FetchContent_MakeAvailable(Catch2)

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
        src/core/Overloaded.h
        src/core/IFileProbe.h
        src/platform/windows/WindowsFileProbe.h
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

# 4. Enable test discovery in your IDE
include(CTest)
include(Catch)
catch_discover_tests(unit_tests)