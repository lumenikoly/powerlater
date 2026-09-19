#include "timer.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <system_error>

namespace power_timer {
namespace {
std::optional<int> number(std::string_view text) {
    if (text.empty() || text.size() > 2) return std::nullopt;
    // from_chars accepts a minus sign, but the UI must accept only digits.
    for (const char c : text) {
        if (c < '0' || c > '9') return std::nullopt;
    }
    int value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) return std::nullopt;
    return value;
}
} // namespace

ClockSample ClockSample::now() {
    return {std::chrono::steady_clock::now(), std::chrono::system_clock::now()};
}

bool Timer::start(Action action, std::chrono::seconds delay, ClockSample now) {
    if (active_ || delay <= std::chrono::seconds::zero() || delay > max_delay) return false;
    action_ = action;
    previous_ = started_ = now;
    deadline_ = now.steady + delay;
    expected_at_ = now.wall + delay;
    active_ = true;
    return true;
}

void Timer::cancel() noexcept { active_ = false; }

Tick Timer::tick(ClockSample now) {
    if (!active_) return Tick::idle;
    const auto steady_gap = now.steady - previous_.steady;
    const auto wall_gap = now.wall - previous_.wall;
    // Compare total elapsed time too, so many small wall-clock changes do not
    // silently turn the displayed target time into a different time.
    const auto drift = (now.wall - started_.wall) - (now.steady - started_.steady);
    if (steady_gap < decltype(steady_gap)::zero() || wall_gap < decltype(wall_gap)::zero() ||
        steady_gap > max_tick_gap || wall_gap > max_tick_gap || drift > max_clock_drift ||
        drift < -max_clock_drift) {
        active_ = false;
        return Tick::interrupted;
    }
    previous_ = now;
    if (now.steady >= deadline_) {
        active_ = false; // Consume once, BEFORE the caller starts the OS operation.
        return Tick::due;
    }
    return Tick::running;
}

std::chrono::seconds Timer::remaining(ClockSample now) const {
    if (!active_ || now.steady >= deadline_) return std::chrono::seconds::zero();
    return std::chrono::ceil<std::chrono::seconds>(deadline_ - now.steady);
}

std::optional<std::chrono::seconds> parse_delay(std::string_view hours, std::string_view minutes) {
    const auto h = number(hours);
    const auto m = number(minutes);
    if (!h || !m || *h > 24 || *m > 59) return std::nullopt;
    const auto delay = std::chrono::hours{*h} + std::chrono::minutes{*m};
    if (delay <= std::chrono::seconds::zero() || delay > Timer::max_delay) return std::nullopt;
    return std::chrono::duration_cast<std::chrono::seconds>(delay);
}

std::string format_remaining(std::chrono::seconds remaining) {
    const auto seconds = std::max<std::int64_t>(0, remaining.count());
    char text[32]{};
    std::snprintf(
        text, sizeof(text), "%02lld:%02lld:%02lld", static_cast<long long>(seconds / 3600),
        static_cast<long long>((seconds % 3600) / 60), static_cast<long long>(seconds % 60));
    return text;
}

std::string format_local_time(std::chrono::system_clock::time_point at) {
    const auto raw = std::chrono::system_clock::to_time_t(at);
    std::tm local{};
#ifdef _WIN32
    if (localtime_s(&local, &raw) != 0) return "—";
#else
    if (!localtime_r(&raw, &local)) return "—";
#endif
    char text[40]{};
    if (std::strftime(text, sizeof(text), "%d.%m, %H:%M:%S", &local) == 0) return "—";
    return text;
}

const char *action_name(Action action) noexcept { return action_name(action, current_locale()); }

const char *action_name(Action action, Locale locale) noexcept {
    return text(locale, action == Action::sleep ? Text::sleep_action : Text::shutdown_action);
}
} // namespace power_timer
