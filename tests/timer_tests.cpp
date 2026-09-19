#include "test.hpp"
#include "timer.hpp"

#include <chrono>

using namespace power_timer;
using namespace std::chrono_literals;

namespace {
ClockSample clock_at(std::chrono::milliseconds monotonic, std::chrono::milliseconds wall) {
    return {std::chrono::steady_clock::time_point{monotonic},
            std::chrono::system_clock::time_point{wall}};
}
ClockSample clock_at(std::chrono::milliseconds elapsed) { return clock_at(elapsed, elapsed); }
} // namespace

int main() {
    int failures = 0;
    test(
        "valid durations",
        [] {
            CHECK(parse_delay("0", "1") == 1min);
            CHECK(parse_delay("0", "30") == 30min);
            CHECK(parse_delay("1", "5") == 65min);
            CHECK(parse_delay("01", "05") == 65min);
            CHECK(parse_delay("23", "59") == 1439min);
            CHECK(parse_delay("24", "0") == 24h);
        },
        failures);
    test(
        "reject invalid and ambiguous input",
        [] {
            for (const auto *hours : {"", "-1", "+1", " 1", "1 ", "1.5", "100", "1a", "０"})
                CHECK(!parse_delay(hours, "30"));
            for (const auto *minutes : {"", "-1", "+1", " 1", "60", "99", "1.5", "1a", "\n"})
                CHECK(!parse_delay("0", minutes));
            CHECK(!parse_delay("0", "0"));
            CHECK(!parse_delay("24", "1"));
            CHECK(!parse_delay("25", "0"));
        },
        failures);
    test(
        "entire valid input range",
        [] {
            for (int hours = 0; hours <= 24; ++hours) {
                for (int minutes = 0; minutes < 60; ++minutes) {
                    const int total = hours * 60 + minutes;
                    const auto parsed = parse_delay(std::to_string(hours), std::to_string(minutes));
                    CHECK(parsed.has_value() == (total > 0 && total <= 1440));
                    if (parsed) CHECK(*parsed == std::chrono::minutes{total});
                }
            }
        },
        failures);
    test(
        "idle timer never fires",
        [] {
            Timer timer;
            CHECK(!timer.active());
            CHECK(timer.tick(clock_at(0s)) == Tick::idle);
            CHECK(timer.tick(clock_at(100h)) == Tick::idle);
            CHECK(timer.remaining(clock_at(0s)) == 0s);
        },
        failures);
    test(
        "invalid start leaves timer idle",
        [] {
            Timer timer;
            CHECK(!timer.start(Action::shutdown, 0s, clock_at(0s)));
            CHECK(!timer.start(Action::shutdown, -1s, clock_at(0s)));
            CHECK(!timer.start(Action::shutdown, 24h + 1s, clock_at(0s)));
            CHECK(!timer.active());
        },
        failures);
    test(
        "deadline and ceiling",
        [] {
            Timer timer;
            CHECK(timer.start(Action::shutdown, 60s, clock_at(10s)));
            CHECK(timer.action() == Action::shutdown);
            CHECK(timer.expected_at() == clock_at(70s).wall);
            CHECK(timer.remaining(clock_at(10s)) == 60s);
            CHECK(timer.remaining(clock_at(10001ms)) == 60s);
            CHECK(timer.remaining(clock_at(11s)) == 59s);
        },
        failures);
    test(
        "exactly one execution",
        [] {
            Timer timer;
            CHECK(timer.start(Action::sleep, 60s, clock_at(0s)));
            for (int seconds = 1; seconds < 60; ++seconds)
                CHECK(timer.tick(clock_at(std::chrono::seconds{seconds})) == Tick::running);
            CHECK(timer.tick(clock_at(60s)) == Tick::due);
            CHECK(!timer.active());
            CHECK(timer.tick(clock_at(61s)) == Tick::idle);
            CHECK(timer.tick(clock_at(90s)) == Tick::idle);
        },
        failures);
    test(
        "late event within tolerance",
        [] {
            Timer timer;
            CHECK(timer.start(Action::shutdown, 2s, clock_at(0s)));
            CHECK(timer.tick(clock_at(2500ms)) == Tick::due);
            CHECK(timer.tick(clock_at(3s)) == Tick::idle);
        },
        failures);
    test(
        "cancel before deadline",
        [] {
            Timer timer;
            CHECK(timer.start(Action::shutdown, 2s, clock_at(0s)));
            timer.cancel();
            timer.cancel();
            CHECK(timer.tick(clock_at(2s)) == Tick::idle);
            CHECK(timer.remaining(clock_at(1s)) == 0s);
        },
        failures);
    test(
        "reject overlapping schedules",
        [] {
            Timer timer;
            CHECK(timer.start(Action::sleep, 2s, clock_at(0s)));
            CHECK(!timer.start(Action::shutdown, 1s, clock_at(0s)));
            CHECK(timer.action() == Action::sleep);
            CHECK(timer.tick(clock_at(2s)) == Tick::due);
        },
        failures);
    test(
        "reschedule after cancellation or completion",
        [] {
            Timer timer;
            CHECK(timer.start(Action::sleep, 2s, clock_at(0s)));
            timer.cancel();
            CHECK(timer.start(Action::shutdown, 3s, clock_at(1s)));
            CHECK(timer.tick(clock_at(4s)) == Tick::due);
            CHECK(timer.start(Action::sleep, 1s, clock_at(5s)));
            CHECK(timer.tick(clock_at(6s)) == Tick::due);
        },
        failures);
    test(
        "suspend where steady clock stops",
        [] {
            Timer timer;
            CHECK(timer.start(Action::shutdown, 60s, clock_at(0s)));
            CHECK(timer.tick(clock_at(1s)) == Tick::running);
            CHECK(timer.tick(clock_at(2s, 3602s)) == Tick::interrupted);
            CHECK(timer.tick(clock_at(60s, 3660s)) == Tick::idle);
        },
        failures);
    test(
        "suspend where both clocks advance",
        [] {
            Timer timer;
            CHECK(timer.start(Action::shutdown, 60s, clock_at(0s)));
            CHECK(timer.tick(clock_at(1h)) == Tick::interrupted);
            CHECK(!timer.active());
        },
        failures);
    test(
        "long event loop pause cancels even before deadline",
        [] {
            Timer timer;
            CHECK(timer.start(Action::shutdown, 24h, clock_at(0s)));
            CHECK(timer.tick(clock_at(11s)) == Tick::interrupted);
        },
        failures);
    test(
        "clock moved backwards",
        [] {
            Timer timer;
            CHECK(timer.start(Action::shutdown, 60s, clock_at(10s)));
            CHECK(timer.tick(clock_at(11s, 1s)) == Tick::interrupted);
        },
        failures);
    test(
        "clock moved forward without long gap",
        [] {
            Timer timer;
            CHECK(timer.start(Action::shutdown, 60s, clock_at(0s)));
            CHECK(timer.tick(clock_at(1s, 8s)) == Tick::interrupted);
        },
        failures);
    test(
        "small correction tolerated",
        [] {
            Timer timer;
            CHECK(timer.start(Action::sleep, 60s, clock_at(0s)));
            CHECK(timer.tick(clock_at(1s, 1100ms)) == Tick::running);
        },
        failures);
    test(
        "cumulative clock drift detected",
        [] {
            Timer timer;
            CHECK(timer.start(Action::shutdown, 60s, clock_at(0s)));
            CHECK(timer.tick(clock_at(1s, 3s)) == Tick::running);
            CHECK(timer.tick(clock_at(2s, 6s)) == Tick::running);
            CHECK(timer.tick(clock_at(3s, 9s)) == Tick::interrupted);
        },
        failures);
    test(
        "broken monotonic clock cancels",
        [] {
            Timer timer;
            CHECK(timer.start(Action::shutdown, 60s, clock_at(10s)));
            CHECK(timer.tick(clock_at(9s, 11s)) == Tick::interrupted);
        },
        failures);
    test(
        "display formatting",
        [] {
            CHECK(format_remaining(-1s) == "00:00:00");
            CHECK(format_remaining(0s) == "00:00:00");
            CHECK(format_remaining(59s) == "00:00:59");
            CHECK(format_remaining(60s) == "00:01:00");
            CHECK(format_remaining(3661s) == "01:01:01");
            CHECK(format_remaining(24h) == "24:00:00");
            CHECK(!format_local_time(clock_at(0s).wall).empty());
        },
        failures);
    return failures == 0 ? 0 : 1;
}
