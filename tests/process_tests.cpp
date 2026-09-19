#include "process.hpp"
#include "test.hpp"

#include <chrono>

using namespace power_timer;
using namespace std::chrono_literals;

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    const std::string fixture = argv[1];
    int failures = 0;
    test(
        "stdout capture",
        [&] {
            const auto result = run_process(fixture, {"success"}, 2s);
            CHECK(result.exit_code == 0);
            CHECK(!result.timed_out);
            CHECK(result.error.empty());
            CHECK(result.output == "ok\n");
        },
        failures);
    test(
        "stderr and exit code",
        [&] {
            const auto result = run_process(fixture, {"error"}, 2s);
            CHECK(result.exit_code == 7);
            CHECK(result.output == "test failure\n");
        },
        failures);
    test(
        "no shell interpretation",
        [&] {
            const auto result = run_process(
                fixture, {"echo", "a b", "; echo bad", "$(echo bad)", "*", "русский"}, 2s);
            CHECK(result.exit_code == 0);
            CHECK(result.output == "a b\n; echo bad\n$(echo bad)\n*\nрусский\n");
        },
        failures);
    test(
        "bounded output without deadlock",
        [&] {
            const auto result = run_process(fixture, {"large"}, 2s);
            CHECK(result.exit_code == 0);
            CHECK(result.output.size() == 8192);
            CHECK(!result.timed_out);
        },
        failures);
    test(
        "process timeout",
        [&] {
            const auto before = std::chrono::steady_clock::now();
            const auto result = run_process(fixture, {"sleep"}, 60ms);
            CHECK(result.timed_out);
            CHECK(std::chrono::steady_clock::now() - before < 1s);
        },
        failures);
    test(
        "closed output does not prevent timeout",
        [&] {
            const auto result = run_process(fixture, {"closed-then-sleep"}, 60ms);
            CHECK(result.timed_out);
        },
        failures);
    test(
        "missing executable",
        [] {
            const auto result = run_process("/nonexistent/power-timer-fixture", {}, 1s);
            CHECK(result.exit_code == -1);
            CHECK(!result.error.empty());
        },
        failures);
    test(
        "reject relative path and invalid timeout",
        [] {
            CHECK(!run_process("echo", {}, 1s).error.empty());
            CHECK(!run_process("/bin/echo", {}, 0ms).error.empty());
        },
        failures);
    return failures == 0 ? 0 : 1;
}
