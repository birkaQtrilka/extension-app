//
// Created by Birka on 15/10/2026.
//

#ifndef IFOLDERWATCHER_H
#define IFOLDERWATCHER_H
#include <functional>
#include <stop_token>

#include "FileEvent.h"


class IFolderWatcher {
public:
    virtual ~IFolderWatcher() = default;
    using EventCallback = std::function<void(FileEvent)>;
    virtual void start(EventCallback onEvent, std::stop_token stop) = 0;
};


#endif //IFOLDERWATCHER_H
