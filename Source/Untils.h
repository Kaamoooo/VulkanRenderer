#pragma once

#include <cstdio>
#include <cstddef>
#include <functional>

namespace Kaamoo {

    template<typename T, typename ... Rest>
    void hashCombine(std::size_t &seed, const T &v, const Rest &...rest) {
        seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        //折叠表达式，"..."表示串联多个函数调用
        (hashCombine(seed, rest), ...);
    }

}