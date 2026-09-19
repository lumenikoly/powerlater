#pragma once
#include <iostream>
#include <stdexcept>
#include <string>

#define CHECK(expression)                                                                          \
    do {                                                                                           \
        if (!(expression))                                                                         \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) +      \
                                     ": " #expression);                                            \
    } while (false)

template <typename Function> void test(const char *name, Function function, int &failures) {
    try {
        function();
        std::cout << "PASS " << name << '\n';
    } catch (const std::exception &error) {
        ++failures;
        std::cerr << "FAIL " << name << ": " << error.what() << '\n';
    }
}
