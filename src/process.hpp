#pragma once
#include "localization.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace power_timer {
struct ProcessResult {
    int exit_code = -1;
    bool timed_out = false;
    std::string output;
    std::string error;
};

// Linux only: absolute executable path, arguments passed directly to posix_spawn.
// No shell, no expansion, bounded output and wait time.
ProcessResult run_process(const std::string &executable, const std::vector<std::string> &arguments,
                          std::chrono::milliseconds timeout, Locale locale = current_locale());
std::string system_executable(const char *name);
} // namespace power_timer
