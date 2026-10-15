//
// Created by Birka on 15/10/2026.
//
#pragma once
#include <windows.h>

struct HandleCloser {
    void operator()(HANDLE h) const {
        CloseHandle(h);
    }
};

using UniqueHandle = std::unique_ptr<void, HandleCloser>;
