#pragma once
#include "timer.hpp"
#include <string>
#include <utility>

namespace power_timer {
struct Result {
    bool ok;
    std::string message;
    static Result success() { return {true, {}}; }
    static Result failure(std::string message) { return {false, std::move(message)}; }
};

// check() must NEVER change the power state. execute() submits exactly one
// request, without force flags, privilege escalation or automatic retries.
Result check_power_action(Action action, Locale locale);
Result execute_power_action(Action action, Locale locale);

// Compatibility helpers use the current locale for callers outside the GUI.
inline Result check_power_action(Action action) {
    return check_power_action(action, current_locale());
}
inline Result execute_power_action(Action action) {
    return execute_power_action(action, current_locale());
}
} // namespace power_timer
