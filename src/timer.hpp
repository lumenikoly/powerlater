#pragma once

#include "localization.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace power_timer {

enum class Action { sleep, shutdown };

struct ClockSample {
    std::chrono::steady_clock::time_point steady;
    std::chrono::system_clock::time_point wall;
    static ClockSample now();
};

enum class Tick { idle, running, due, interrupted };

// No system calls here. Tests supply a clock instead of waiting in real time.
class Timer {
  public:
    bool start(Action action, std::chrono::seconds delay, ClockSample now);
    void cancel() noexcept;
    Tick tick(ClockSample now);
    bool active() const noexcept { return active_; }
    Action action() const noexcept { return action_; }
    std::chrono::seconds remaining(ClockSample now) const;
    std::chrono::system_clock::time_point expected_at() const { return expected_at_; }

    static constexpr auto max_delay = std::chrono::hours{24};
    // Cancel rather than surprise the user after suspend or a stalled event loop.
    static constexpr auto max_tick_gap = std::chrono::seconds{10};
    static constexpr auto max_clock_drift = std::chrono::seconds{5};

  private:
    bool active_ = false;
    Action action_ = Action::sleep;
    ClockSample previous_{};
    ClockSample started_{};
    std::chrono::steady_clock::time_point deadline_{};
    std::chrono::system_clock::time_point expected_at_{};
};

std::optional<std::chrono::seconds> parse_delay(std::string_view hours, std::string_view minutes);
std::string format_remaining(std::chrono::seconds remaining);
std::string format_local_time(std::chrono::system_clock::time_point at);
const char *action_name(Action action) noexcept;
const char *action_name(Action action, Locale locale) noexcept;

} // namespace power_timer
