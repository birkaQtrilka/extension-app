//
// Created by Birka on 15/10/2026.
//
#ifndef FILEEVENT_H
#define FILEEVENT_H
#pragma once
#include <filesystem>
#include <variant>


struct Modified { std::filesystem::path path; };
struct Created  { std::filesystem::path path; };
struct Removed  { std::filesystem::path path; };
struct Overflow  {};
struct Renamed  { std::filesystem::path from; std::filesystem::path to; };
struct FileReady { std::filesystem::path path; };

using FileEvent = std::variant<Created, Modified, Removed, Renamed, Overflow>;

#endif //FILEEVENT_H
