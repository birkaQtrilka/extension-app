cmake_minimum_required(VERSION 3.30)
project(extension_app)
set(CMAKE_CXX_STANDARD 26)

add_executable(extension_app main.cpp
        src/core/IAction.cpp
        src/core/IAction.h
        src/core/IFolderWatcher.cpp
        src/core/IFolderWatcher.h
        src/core/FileEvent.h
        src/core/ActionError.h)
