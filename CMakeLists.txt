cmake_minimum_required(VERSION 3.30)
project(extension_app)
set(CMAKE_CXX_STANDARD 26)

add_executable(extension_app main.cpp
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
        src/platform/windows/WindowsFileProbe.h)
