#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char **argv) {
    if (argc < 2) return 2;
    const std::string mode = argv[1];
    if (mode == "success") {
        std::cout << "ok\n";
        return 0;
    }
    if (mode == "error") {
        std::cerr << "test failure\n";
        return 7;
    }
    if (mode == "echo") {
        for (int i = 2; i < argc; ++i)
            std::cout << argv[i] << '\n';
        return 0;
    }
    if (mode == "large") {
        std::cout << std::string(100000, 'x');
        return 0;
    }
    if (mode == "sleep") {
        std::this_thread::sleep_for(std::chrono::seconds{2});
        return 0;
    }
    if (mode == "closed-then-sleep") {
        std::fclose(stdout);
        std::fclose(stderr);
        std::this_thread::sleep_for(std::chrono::seconds{2});
        return 0;
    }
    return 2;
}
