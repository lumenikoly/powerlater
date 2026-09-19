#include "localization.hpp"
#include "test.hpp"
#include "timer.hpp"

#include <string>

using namespace power_timer;

int main() {
    int failures = 0;
    test(
        "English is the default locale",
        [] {
            CHECK(current_locale() == Locale::en);
            CHECK(std::string(text(Locale::en, Text::app_title)) == "Power Timer");
            CHECK(std::string(action_name(Action::sleep, Locale::en)) == "Sleep");
        },
        failures);
    test(
        "Russian strings are available",
        [] {
            CHECK(std::string(text(Locale::ru, Text::app_title)) == "Таймер питания");
            CHECK(std::string(action_name(Action::shutdown, Locale::ru)) == "Выключить");
            CHECK(std::string(text(Locale::ru, Text::linux_timeout)).find("истекло") !=
                  std::string::npos);
        },
        failures);
    test(
        "locale changes are atomic",
        [] {
            set_locale(Locale::ru);
            CHECK(current_locale() == Locale::ru);
            CHECK(std::string(action_name(Action::sleep, Locale::en)) == "Sleep");
            set_locale(Locale::en);
            CHECK(current_locale() == Locale::en);
        },
        failures);
    test(
        "every catalog key has both translations",
        [] {
            for (int value = 0; value < static_cast<int>(Text::count); ++value) {
                const auto key = static_cast<Text>(value);
                CHECK(text(Locale::en, key)[0] != '\0');
                CHECK(text(Locale::ru, key)[0] != '\0');
            }
        },
        failures);
    return failures == 0 ? 0 : 1;
}
