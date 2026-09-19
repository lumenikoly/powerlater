#include "activity.hpp"
#include "licenses.hpp"
#include "localization.hpp"
#include "power.hpp"
#include "timer.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Toggle_Button.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>

#include <array>
#include <atomic>
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace power_timer {
namespace {
constexpr int width = 480;
constexpr int height = 610;
constexpr int margin = 24;
constexpr int content_width = width - 2 * margin;
const Fl_Color background = fl_rgb_color(244, 246, 250);
const Fl_Color foreground = fl_rgb_color(30, 39, 59);
const Fl_Color muted = fl_rgb_color(103, 114, 134);
const Fl_Color accent = fl_rgb_color(76, 98, 218);
const Fl_Color selected = fl_rgb_color(230, 235, 255);
const Fl_Color subtle = fl_rgb_color(242, 244, 249);
constexpr Fl_Boxtype rounded_box = static_cast<Fl_Boxtype>(FL_FREE_BOXTYPE);
constexpr Fl_Boxtype input_box = static_cast<Fl_Boxtype>(FL_FREE_BOXTYPE + 1);

void draw_rounded_box(int x, int y, int w, int h, Fl_Color color) {
    Fl::set_box_color(color);
    fl_rounded_rectf(x, y, w, h, 14);
}

void draw_input_box(int x, int y, int w, int h, Fl_Color color) {
    draw_rounded_box(x, y, w, h, color);
    fl_color(fl_rgb_color(224, 229, 239));
    fl_rounded_rect(x, y, w, h, 12);
}

class PowerMark final : public Fl_Box {
  public:
    PowerMark(int x, int y) : Fl_Box(x, y, 40, 40) {}
    void draw() override {
        fl_color(selected);
        fl_rounded_rectf(x(), y(), w(), h(), 13);
        fl_color(accent);
        fl_line_style(FL_SOLID, 2);
        fl_arc(x() + 11, y() + 11, 18, 18, 135, 405);
        fl_line(x() + 20, y() + 9, x() + 20, y() + 19);
        fl_line_style(FL_SOLID);
    }
};

// The worker owns this record, not the window. It can finish safely if the user
// closes the app while checking permissions. The GUI is the only result reader.
struct Operation {
    std::atomic<bool> ready{false};
    Result result = Result::success();
};

enum class Phase { idle, checking, scheduled, submitting };

class App final : public Fl_Double_Window {
  public:
    explicit App(bool dry_run, bool smoke_test)
        : Fl_Double_Window(width, height, power_timer::text(current_locale(), Text::app_title)),
          dry_run_(dry_run), smoke_test_(smoke_test), locale_(current_locale()) {
        color(background);
        Fl::set_boxtype(rounded_box, draw_rounded_box, 2, 2, 4, 4);
        Fl::set_boxtype(input_box, draw_input_box, 12, 2, 24, 4);
        size_range(width, height, width, height);
        callback([](Fl_Widget *, void *self) { static_cast<App *>(self)->close_app(); }, this);
        begin();
        new PowerMark(margin, 29);
        title_ = text(margin + 54, 26, content_width - 116, 30,
                      power_timer::text(locale_, Text::app_title), 22);
        title_->labelfont(FL_HELVETICA_BOLD);
        subtitle_ = text(
            margin + 54, 59, content_width - 54, 24,
            power_timer::text(locale_, dry_run_ ? Text::subtitle_dry_run : Text::subtitle_normal),
            12, muted);
        language_button_ = new Fl_Button(width - margin - 50, 29, 50, 32,
                                         power_timer::text(locale_, Text::language_button));
        language_button_->box(rounded_box);
        language_button_->color(FL_WHITE);
        language_button_->selection_color(selected);
        language_button_->labelcolor(muted);
        language_button_->labelsize(12);
        language_button_->callback(
            [](Fl_Widget *, void *self) { static_cast<App *>(self)->toggle_locale(); }, this);

        auto *card = new Fl_Box(rounded_box, margin, 108, content_width, 386, "");
        card->color(FL_WHITE);

        controls_ = new Fl_Group(margin + 20, 128, content_width - 40, 342);
        controls_->begin();
        actions_ = new Fl_Group(margin + 20, 128, content_width - 40, 44);
        actions_->box(rounded_box);
        actions_->color(subtle);
        actions_->begin();
        sleep_ =
            action_button(margin + 24, 132, 190, power_timer::text(locale_, Text::sleep_action));
        shutdown_ = action_button(margin + 218, 132, 190,
                                  power_timer::text(locale_, Text::shutdown_action));
        sleep_->value(1);
        actions_->end();
        through_ =
            text(margin + 24, 353, 65, 44, power_timer::text(locale_, Text::through), 14, muted);
        hours_ = new Fl_Int_Input(margin + 95, 353, 76, 44);
        minutes_ = new Fl_Int_Input(margin + 237, 353, 76, 44);
        for (auto *input : {hours_, minutes_}) {
            input->textsize(21);
            input->textcolor(foreground);
            input->color(subtle);
            input->box(input_box);
            input->selection_color(selected);
            input->maximum_size(2);
            input->when(FL_WHEN_CHANGED);
            input->callback(
                [](Fl_Widget *, void *self) { static_cast<App *>(self)->update_preview(); }, this);
        }
        hours_->value("0");
        minutes_->value("30");
        hours_unit_ = text(margin + 183, 353, 40, 44, power_timer::text(locale_, Text::hours_short),
                           13, muted);
        minutes_unit_ = text(margin + 325, 353, 65, 44,
                             power_timer::text(locale_, Text::minutes_short), 13, muted);

        const std::array<const char *, 4> labels{power_timer::text(locale_, Text::preset_15),
                                                 power_timer::text(locale_, Text::preset_30),
                                                 power_timer::text(locale_, Text::preset_1_hour),
                                                 power_timer::text(locale_, Text::preset_2_hours)};
        for (std::size_t i = 0; i < presets_.size(); ++i) {
            presets_[i] =
                new Fl_Button(margin + 24 + static_cast<int>(i) * 98, 417, 90, 36, labels[i]);
            presets_[i]->labelsize(13);
            presets_[i]->box(rounded_box);
            presets_[i]->color(subtle);
            presets_[i]->selection_color(selected);
            presets_[i]->labelcolor(muted);
            presets_[i]->callback(
                [](Fl_Widget *widget, void *self) {
                    static_cast<App *>(self)->choose_preset(widget);
                },
                this);
        }
        controls_->end();

        status_ = text(margin + 20, 195, content_width - 40, 24,
                       power_timer::text(locale_, Text::timer_not_started), 12, muted);
        status_->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
        countdown_ = text(margin + 20, 222, content_width - 40, 70, "00:30:00", 56);
        countdown_->labelfont(FL_HELVETICA);
        countdown_->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
        detail_ = text(margin + 24, 291, content_width - 48, 38,
                       power_timer::text(locale_, Text::invalid_interval), 12, muted);
        detail_->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);

        main_button_ = new Fl_Button(margin, 514, content_width, 52,
                                     power_timer::text(locale_, Text::start_timer));
        main_button_->box(rounded_box);
        main_button_->color(accent);
        main_button_->selection_color(fl_rgb_color(60, 79, 192));
        main_button_->labelcolor(FL_WHITE);
        main_button_->labelfont(FL_HELVETICA_BOLD);
        main_button_->labelsize(16);
        main_button_->callback(
            [](Fl_Widget *, void *self) { static_cast<App *>(self)->main_action(); }, this);
        footer_ = text(margin, 577, content_width, 24,
                       power_timer::text(locale_, dry_run_ ? Text::dry_run_footer_idle
                                                           : Text::normal_footer_idle),
                       11, muted);
        footer_->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
        end();
        update_preview();
    }

    ~App() override { Fl::remove_timeout(tick_callback, this); }
    int exit_code() const noexcept { return exit_code_; }

    void start_smoke_test() {
        if (dry_run_ && smoke_test_) begin_check(std::chrono::seconds{2});
    }

    int handle(int event) override {
        if (event == FL_SHORTCUT && Fl::event_key() == FL_Escape) {
            if (phase_ == Phase::scheduled) cancel_timer();
            return 1; // Escape must not unexpectedly close the window.
        }
        return Fl_Double_Window::handle(event);
    }

  private:
    Fl_Box *text(int x, int y, int w, int h, const char *label, int size,
                 Fl_Color color = foreground) {
        auto *widget = new Fl_Box(x, y, w, h, label);
        widget->labelsize(size);
        widget->labelcolor(color);
        widget->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        return widget;
    }

    Fl_Toggle_Button *action_button(int x, int y, int w, const char *label) {
        auto *button = new Fl_Toggle_Button(x, y, w, 36, label);
        button->type(FL_RADIO_BUTTON);
        button->box(rounded_box);
        button->down_box(rounded_box);
        button->labelsize(14);
        button->color(subtle);
        button->labelcolor(foreground);
        button->selection_color(selected);
        button->callback(
            [](Fl_Widget *, void *self) { static_cast<App *>(self)->update_preview(); }, this);
        return button;
    }

    Action selected_action() const { return shutdown_->value() ? Action::shutdown : Action::sleep; }

    void choose_preset(Fl_Widget *widget) {
        constexpr std::array<int, 4> minutes{15, 30, 60, 120};
        for (std::size_t i = 0; i < presets_.size(); ++i) {
            if (presets_[i] != widget) continue;
            hours_->value(std::to_string(minutes[i] / 60).c_str());
            minutes_->value(std::to_string(minutes[i] % 60).c_str());
            update_preview();
            break;
        }
    }

    void update_preview() {
        if (phase_ != Phase::idle) return;
        sleep_->labelcolor(selected_action() == Action::sleep ? accent : muted);
        shutdown_->labelcolor(selected_action() == Action::shutdown ? accent : muted);
        const auto delay = parse_delay(hours_->value(), minutes_->value());
        constexpr std::array<int, 4> preset_minutes{15, 30, 60, 120};
        for (std::size_t i = 0; i < presets_.size(); ++i) {
            const bool chosen = delay && *delay == std::chrono::minutes{preset_minutes[i]};
            presets_[i]->color(chosen ? selected : subtle);
            presets_[i]->labelcolor(chosen ? accent : muted);
            presets_[i]->redraw();
        }
        if (!delay) {
            main_button_->deactivate();
            countdown_->copy_label("--:--:--");
            set_detail(Text::invalid_interval);
        } else {
            main_button_->activate();
            countdown_->copy_label(format_remaining(*delay).c_str());
            set_detail(selected_action() == Action::shutdown ? Text::shutdown_detail
                                                             : Text::sleep_detail);
        }
    }

    void main_action() {
        if (phase_ == Phase::scheduled) {
            cancel_timer();
        } else if (phase_ == Phase::idle) {
            const auto delay = parse_delay(hours_->value(), minutes_->value());
            if (delay) begin_check(*delay);
        }
    }

    void begin_check(std::chrono::seconds delay) {
        pending_action_ = selected_action();
        pending_delay_ = delay;
        controls_->deactivate();
        phase_ = Phase::checking;
        language_button_->deactivate();
        main_button_->deactivate();
        main_button_->copy_label(power_timer::text(locale_, Text::checking_button));
        set_status(Text::checking_status);
        set_detail(Text::checking_detail);
        start_operation(true);
    }

    void start_operation(bool check_only) {
        operation_ = std::make_shared<Operation>();
        try {
            // The worker captures no widgets and no `this` pointer. Only the UI
            // thread may arm a timer after a successful, non-destructive check.
            std::thread([operation = operation_, action = pending_action_, check_only,
                         dry_run = dry_run_, locale = locale_]() {
                try {
                    operation->result = dry_run
                                            ? Result::success()
                                            : (check_only ? check_power_action(action, locale)
                                                          : execute_power_action(action, locale));
                } catch (const std::exception &error) {
                    operation->result = Result::failure(error.what());
                } catch (...) {
                    operation->result =
                        Result::failure(power_timer::text(locale, Text::unknown_system_error));
                }
                operation->ready.store(true, std::memory_order_release);
            }).detach();
        } catch (const std::exception &error) {
            operation_->result = Result::failure(error.what());
            operation_->ready.store(true, std::memory_order_release);
        }
        schedule_tick(0.1);
    }

    void schedule_tick(double seconds) {
        Fl::remove_timeout(tick_callback, this);
        Fl::add_timeout(seconds, tick_callback, this);
    }

    static void tick_callback(void *data) { static_cast<App *>(data)->tick(); }

    void tick() {
        if (phase_ == Phase::checking || phase_ == Phase::submitting) {
            if (!operation_->ready.load(std::memory_order_acquire)) {
                schedule_tick(0.1);
                return;
            }
            const Result result = operation_->result;
            operation_.reset();
            if (!result.ok) {
                set_idle(Text::action_failed);
                if (smoke_test_) {
                    exit_code_ = 1;
                    hide();
                } else
                    fl_alert("%s", result.message.c_str());
                return;
            }
            if (phase_ == Phase::checking) {
                if (!timer_.start(pending_action_, pending_delay_, ClockSample::now())) {
                    set_idle(Text::timer_start_failed);
                    return;
                }
                phase_ = Phase::scheduled;
                activity_.start();
                attention_requested_ = false;
                main_button_->activate();
                main_button_->copy_label(power_timer::text(locale_, Text::cancel_timer_button));
                set_status(Text::timer_started);
                set_detail((std::string(action_name(pending_action_, locale_)) + " · " +
                            format_local_time(timer_.expected_at()))
                               .c_str());
                show_remaining();
                schedule_tick(1.0);
            } else {
                set_idle(dry_run_ ? Text::dry_run_finished : Text::request_submitted);
                set_detail(dry_run_ ? Text::dry_run_detail_finished : Text::normal_detail_finished);
                if (smoke_test_) hide();
            }
            return;
        }
        if (phase_ != Phase::scheduled) return;
        switch (timer_.tick(ClockSample::now())) {
        case Tick::interrupted:
            set_idle(Text::interrupted_status);
            set_detail(Text::interrupted_detail);
            show();
            break;
        case Tick::due:
            activity_.stop();
            phase_ = Phase::submitting;
            set_status(dry_run_ ? Text::checking_trigger : Text::sending_request);
            countdown_->copy_label("00:00:00");
            main_button_->copy_label(power_timer::text(locale_, Text::request_sending_button));
            main_button_->deactivate();
            set_detail(Text::cancellation_unavailable);
            footer_->copy_label(power_timer::text(locale_, dry_run_ ? Text::dry_run_footer_due
                                                                    : Text::normal_footer_due));
            start_operation(false);
            break;
        case Tick::running:
            show_remaining();
            schedule_tick(1.0);
            break;
        case Tick::idle:
            break;
        }
    }

    void show_remaining() {
        const auto remaining = timer_.remaining(ClockSample::now());
        const auto formatted = format_remaining(remaining);
        countdown_->copy_label(formatted.c_str());
        copy_label(((dry_run_ ? std::string(power_timer::text(locale_, Text::dry_run_action_prefix))
                              : "") +
                    std::string(action_name(pending_action_, locale_)) +
                    power_timer::text(locale_, Text::action_through) + formatted)
                       .c_str());
        if (remaining <= std::chrono::seconds{30} && !attention_requested_) {
            attention_requested_ = true;
            set_status(Text::remaining_30);
            if (!smoke_test_) show();
        }
    }

    void set_status(Text key) {
        status_key_ = key;
        status_->copy_label(power_timer::text(locale_, key));
    }

    void set_detail(Text key) { detail_->copy_label(power_timer::text(locale_, key)); }

    void set_detail(const char *value) { detail_->copy_label(value); }

    void toggle_locale() {
        if (phase_ != Phase::idle) return;
        locale_ = locale_ == Locale::en ? Locale::ru : Locale::en;
        set_locale(locale_);
        title_->copy_label(power_timer::text(locale_, Text::app_title));
        subtitle_->copy_label(
            power_timer::text(locale_, dry_run_ ? Text::subtitle_dry_run : Text::subtitle_normal));
        language_button_->copy_label(power_timer::text(locale_, Text::language_button));
        sleep_->copy_label(power_timer::text(locale_, Text::sleep_action));
        shutdown_->copy_label(power_timer::text(locale_, Text::shutdown_action));
        through_->copy_label(power_timer::text(locale_, Text::through));
        hours_unit_->copy_label(power_timer::text(locale_, Text::hours_short));
        minutes_unit_->copy_label(power_timer::text(locale_, Text::minutes_short));
        const std::array<Text, 4> preset_keys{Text::preset_15, Text::preset_30, Text::preset_1_hour,
                                              Text::preset_2_hours};
        for (std::size_t i = 0; i < presets_.size(); ++i)
            presets_[i]->copy_label(power_timer::text(locale_, preset_keys[i]));
        copy_label(power_timer::text(locale_, dry_run_ ? Text::dry_run_title : Text::normal_title));
        main_button_->copy_label(power_timer::text(locale_, Text::start_timer));
        footer_->copy_label(power_timer::text(locale_, dry_run_ ? Text::dry_run_footer_idle
                                                                : Text::normal_footer_idle));
        set_status(status_key_);
        update_preview();
    }

    void set_idle(Text message) {
        timer_.cancel();
        activity_.stop();
        phase_ = Phase::idle;
        Fl::remove_timeout(tick_callback, this);
        copy_label(power_timer::text(locale_, dry_run_ ? Text::dry_run_title : Text::normal_title));
        controls_->activate();
        language_button_->activate();
        main_button_->copy_label(power_timer::text(locale_, Text::start_timer));
        set_status(message);
        footer_->copy_label(power_timer::text(locale_, dry_run_ ? Text::dry_run_footer_idle
                                                                : Text::normal_footer_idle));
        update_preview();
    }

    void cancel_timer() { set_idle(Text::timer_cancelled); }

    void close_app() {
        timer_.cancel();
        activity_.stop();
        Fl::remove_timeout(tick_callback, this);
        hide();
    }

    bool dry_run_;
    bool smoke_test_;
    Locale locale_;
    int exit_code_ = 0;
    Phase phase_ = Phase::idle;
    Timer timer_;
    TimerActivity activity_;
    Action pending_action_ = Action::sleep;
    std::chrono::seconds pending_delay_{0};
    bool attention_requested_ = false;
    std::shared_ptr<Operation> operation_;
    Fl_Group *controls_ = nullptr;
    Fl_Group *actions_ = nullptr;
    Fl_Box *title_ = nullptr;
    Fl_Box *subtitle_ = nullptr;
    Fl_Box *through_ = nullptr;
    Fl_Box *hours_unit_ = nullptr;
    Fl_Box *minutes_unit_ = nullptr;
    Fl_Button *language_button_ = nullptr;
    Fl_Toggle_Button *sleep_ = nullptr;
    Fl_Toggle_Button *shutdown_ = nullptr;
    Fl_Int_Input *hours_ = nullptr;
    Fl_Int_Input *minutes_ = nullptr;
    std::array<Fl_Button *, 4> presets_{};
    Fl_Box *status_ = nullptr;
    Fl_Box *countdown_ = nullptr;
    Fl_Box *detail_ = nullptr;
    Fl_Box *footer_ = nullptr;
    Fl_Button *main_button_ = nullptr;
    Text status_key_ = Text::timer_not_started;
};
} // namespace
} // namespace power_timer

int main(int argc, char **argv) {
    bool dry_run = false;
    bool smoke_test = false;
    power_timer::Locale locale = power_timer::Locale::en;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--dry-run")
            dry_run = true;
        else if (argument == "--smoke-test")
            smoke_test = true;
        else if (argument == "--license") {
            std::cout << power_timer::license_notices;
            return 0;
        } else if (argument == "--lang") {
            if (i + 1 >= argc) {
                std::cerr << power_timer::text(locale, power_timer::Text::unknown_argument)
                          << "--lang\n";
                return 2;
            }
            const std::string language = argv[++i];
            if (language == "en")
                locale = power_timer::Locale::en;
            else if (language == "ru")
                locale = power_timer::Locale::ru;
            else {
                std::cerr << power_timer::text(locale, power_timer::Text::unknown_argument)
                          << "--lang " << language << '\n';
                return 2;
            }
            power_timer::set_locale(locale);
        } else if (argument == "--help") {
            std::cout << power_timer::text(locale, power_timer::Text::help_text);
            return 0;
        } else {
            std::cerr << power_timer::text(locale, power_timer::Text::unknown_argument) << argument
                      << '\n';
            return 2;
        }
    }
    if (smoke_test && !dry_run) {
        std::cerr << power_timer::text(locale, power_timer::Text::smoke_requires_dry_run);
        return 2;
    }
    Fl::scheme("base");
#ifdef _WIN32
    Fl::set_font(FL_HELVETICA, " Segoe UI");
    Fl::set_font(FL_HELVETICA_BOLD, " Segoe UI Semibold");
#endif
    Fl::background(244, 246, 250);
    Fl::foreground(30, 39, 59);
    fl_message_font(FL_HELVETICA, 14);
    try {
        power_timer::App app(dry_run, smoke_test);
        app.show();
        app.start_smoke_test();
        Fl::run();
        return app.exit_code();
    } catch (const std::exception &error) {
        fl_alert("%s%s", power_timer::text(locale, power_timer::Text::startup_error), error.what());
        return 1;
    }
}
