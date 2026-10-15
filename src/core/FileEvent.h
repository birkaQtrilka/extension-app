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
struct Renamed  { std::filesystem::path from; std::filesystem::path to; };

using FileEvent = std::variant<Created, Modified, Removed, Renamed>;

#endif //FILEEVENT_H
