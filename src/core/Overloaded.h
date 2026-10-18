//
// Created by Birka on 18/10/2026.
//

#ifndef OVERLOADED_H
#define OVERLOADED_H
template <class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

#endif //OVERLOADED_H
