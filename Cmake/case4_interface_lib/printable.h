#ifndef PRINTABLE_H
#define PRINTABLE_H

#include <cstdio>
#include <string>

// INTERFACE 库典型场景：纯模板 / 纯头文件
// 不需要 .cpp，所有实现都在头文件里

template <typename T>
void print_value(const T& val) {
    printf("Value: %s\n", std::to_string(val).c_str());
}

// 特化版本
template <>
inline void print_value(const std::string& val) {
    printf("Value: %s\n", val.c_str());
}

#endif
