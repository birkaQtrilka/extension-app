//
// Created by Birka on 15/10/2026.
//

#ifndef IFOLDERWATCHER_H
#define IFOLDERWATCHER_H
#include <stop_token>


class IFolderWatcher {
public:
    virtual ~IFolderWatcher() = default;
    virtual void start(std::stop_token stop) = 0;
};


#endif //IFOLDERWATCHER_H
