#include "power.hpp"
#include "process.hpp"

#include <unistd.h>

namespace power_timer {
namespace {
Result process_error(const ProcessResult &result, Locale locale) {
    if (result.timed_out) {
        return Result::failure(std::string(text(locale, Text::linux_timeout)) +
                               text(locale, Text::linux_timeout_followup) +
                               text(locale, Text::linux_no_retry));
    }
    std::string detail = result.error.empty() ? result.output : result.error;
    if (detail.empty())
        detail = std::string(text(locale, Text::exit_code)) + std::to_string(result.exit_code);
    return Result::failure(std::string(text(locale, Text::linux_system_rejected)) + detail);
}
} // namespace

Result check_power_action(Action action, Locale locale) {
    if (::geteuid() == 0) {
        return Result::failure(text(locale, Text::linux_root));
    }
    const auto busctl = system_executable("busctl");
    if (busctl.empty() || system_executable("systemctl").empty()) {
        return Result::failure(text(locale, Text::linux_dependencies));
    }
    // Validate the required systemctl option harmlessly before arming a timer.
    const auto version =
        run_process(system_executable("systemctl"), {"--check-inhibitors=yes", "--version"},
                    std::chrono::seconds{3}, locale);
    if (version.exit_code != 0 || version.timed_out || !version.error.empty())
        return Result::failure(std::string(text(locale, Text::linux_systemd_version)) +
                               version.output + version.error);
    const auto result = run_process(
        busctl,
        {"--system", "--timeout=5s", "--allow-interactive-authorization=no", "call",
         "org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager",
         action == Action::sleep ? "CanSuspend" : "CanPowerOff"},
        std::chrono::seconds{6}, locale);
    if (result.exit_code != 0 || result.timed_out || !result.error.empty())
        return process_error(result, locale);
    auto value = result.output;
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r'))
        value.pop_back();
    if (value == "s \"yes\"") return Result::success();
    if (value == "s \"challenge\"") {
        return Result::failure(text(locale, Text::linux_authorization));
    }
    return Result::failure(std::string(text(locale, Text::linux_no_access)) + value);
}

Result execute_power_action(Action action, Locale locale) {
    const auto available = check_power_action(action, locale);
    if (!available.ok) return available;
    const auto result = run_process(system_executable("systemctl"),
                                    {"--no-ask-password", "--check-inhibitors=yes",
                                     action == Action::sleep ? "suspend" : "poweroff"},
                                    std::chrono::seconds{12}, locale);
    if (result.exit_code != 0 || result.timed_out || !result.error.empty())
        return process_error(result, locale);
    return Result::success();
}
} // namespace power_timer
