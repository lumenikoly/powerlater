use std::time::Duration;

pub const MAX_DELAY: Duration = Duration::from_secs(24 * 60 * 60);
pub const MAX_TICK_GAP: Duration = Duration::from_secs(10);
pub const MAX_CLOCK_DRIFT: Duration = Duration::from_secs(5);

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Action {
    Sleep,
    Shutdown,
}

#[derive(Clone, Copy, Debug)]
pub struct ClockSample {
    pub steady: Duration,
    pub wall: Duration,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Tick {
    Idle,
    Running,
    Due,
    Interrupted,
}

#[derive(Debug, Default)]
pub struct Timer {
    active: bool,
    action: Option<Action>,
    previous: Option<ClockSample>,
    started: Option<ClockSample>,
    deadline: Duration,
    expected_at: Duration,
}

impl Timer {
    pub fn start(&mut self, action: Action, delay: Duration, now: ClockSample) -> bool {
        if self.active || delay.is_zero() || delay > MAX_DELAY {
            return false;
        }
        self.active = true;
        self.action = Some(action);
        self.previous = Some(now);
        self.started = Some(now);
        self.deadline = now.steady.saturating_add(delay);
        self.expected_at = now.wall.saturating_add(delay);
        true
    }

    pub fn cancel(&mut self) {
        self.active = false;
    }
    pub fn active(&self) -> bool {
        self.active
    }
    pub fn expected_at(&self) -> Duration {
        self.expected_at
    }

    pub fn tick(&mut self, now: ClockSample) -> Tick {
        if !self.active {
            return Tick::Idle;
        }
        let previous = self.previous.expect("active timer has a previous sample");
        let started = self.started.expect("active timer has a start sample");
        let steady_gap = now.steady.checked_sub(previous.steady);
        let wall_gap = now.wall.checked_sub(previous.wall);
        let total_steady = now.steady.checked_sub(started.steady);
        let total_wall = now.wall.checked_sub(started.wall);
        let drift_ok = match (total_steady, total_wall) {
            (Some(a), Some(b)) => a.abs_diff(b) <= MAX_CLOCK_DRIFT,
            _ => false,
        };
        if steady_gap.is_none()
            || wall_gap.is_none()
            || steady_gap > Some(MAX_TICK_GAP)
            || wall_gap > Some(MAX_TICK_GAP)
            || !drift_ok
        {
            self.active = false;
            return Tick::Interrupted;
        }
        self.previous = Some(now);
        if now.steady >= self.deadline {
            self.active = false;
            Tick::Due
        } else {
            Tick::Running
        }
    }

    pub fn remaining(&self, now: ClockSample) -> Duration {
        if !self.active {
            return Duration::ZERO;
        }
        self.deadline.saturating_sub(now.steady)
    }
}

pub fn parse_delay(hours: &str, minutes: &str) -> Option<Duration> {
    fn number(value: &str) -> Option<u64> {
        if value.is_empty() || value.len() > 2 || !value.bytes().all(|b| b.is_ascii_digit()) {
            return None;
        }
        value.parse().ok()
    }
    let hours = number(hours)?;
    let minutes = number(minutes)?;
    if hours > 24 || minutes > 59 {
        return None;
    }
    let delay = Duration::from_secs((hours * 60 + minutes) * 60);
    (!delay.is_zero() && delay <= MAX_DELAY).then_some(delay)
}

pub fn format_remaining(duration: Duration) -> String {
    let seconds = duration.as_secs() + u64::from(duration.subsec_nanos() != 0);
    format!(
        "{:02}:{:02}:{:02}",
        seconds / 3600,
        (seconds % 3600) / 60,
        seconds % 60
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    fn at(steady: u64, wall: u64) -> ClockSample {
        ClockSample {
            steady: Duration::from_secs(steady),
            wall: Duration::from_secs(wall),
        }
    }

    #[test]
    fn parses_only_unambiguous_valid_intervals() {
        assert_eq!(parse_delay("0", "1"), Some(Duration::from_secs(60)));
        assert_eq!(parse_delay("24", "0"), Some(MAX_DELAY));
        for (h, m) in [("", "1"), ("-1", "1"), ("0", "0"), ("24", "1"), ("1", "60")] {
            assert_eq!(parse_delay(h, m), None);
        }
    }

    #[test]
    fn timer_fires_once() {
        let mut timer = Timer::default();
        assert!(timer.start(Action::Sleep, Duration::from_secs(2), at(0, 0)));
        assert_eq!(timer.tick(at(1, 1)), Tick::Running);
        assert_eq!(timer.tick(at(2, 2)), Tick::Due);
        assert_eq!(timer.tick(at(3, 3)), Tick::Idle);
    }

    #[test]
    fn interruption_and_clock_drift_cancel() {
        let mut timer = Timer::default();
        assert!(timer.start(Action::Shutdown, Duration::from_secs(60), at(0, 0)));
        assert_eq!(timer.tick(at(11, 11)), Tick::Interrupted);
        assert!(timer.start(Action::Shutdown, Duration::from_secs(60), at(20, 20)));
        assert_eq!(timer.tick(at(21, 27)), Tick::Interrupted);
    }

    #[test]
    fn formats_countdown() {
        assert_eq!(format_remaining(Duration::from_millis(1)), "00:00:01");
        assert_eq!(format_remaining(Duration::from_secs(3661)), "01:01:01");
        assert_eq!(format_remaining(MAX_DELAY), "24:00:00");
    }
}
