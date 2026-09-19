#pragma once

namespace power_timer {
#ifdef __APPLE__
void *begin_timer_activity();
void end_timer_activity(void *token);
#endif

// Keep macOS from applying App Nap while counting down, but do NOT prevent
// normal system/display sleep. On other platforms there is nothing to acquire.
class TimerActivity {
  public:
    TimerActivity() = default;
    ~TimerActivity() { stop(); }
    TimerActivity(const TimerActivity &) = delete;
    TimerActivity &operator=(const TimerActivity &) = delete;
    void start() {
#ifdef __APPLE__
        if (!token_) token_ = begin_timer_activity();
#endif
    }
    void stop() {
#ifdef __APPLE__
        if (token_) {
            end_timer_activity(token_);
            token_ = nullptr;
        }
#endif
    }

  private:
#ifdef __APPLE__
    void *token_ = nullptr;
#endif
};
} // namespace power_timer
