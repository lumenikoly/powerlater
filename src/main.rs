#![cfg_attr(windows, windows_subsystem = "windows")]

use chrono::{DateTime, Local};
use power_timer::localization::{Locale, strings};
use power_timer::platform;
use power_timer::timer::{
    Action, ClockSample, Tick, Timer as CountdownTimer, format_remaining, parse_delay,
};
use slint::{ComponentHandle, ModelRc, SharedString, Timer, TimerMode, VecModel};
use std::cell::RefCell;
use std::rc::Rc;
use std::sync::mpsc::{self, Receiver};
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

slint::include_modules!();

const LICENSES: &str = concat!(
    include_str!("../LICENSE"),
    "\n\n",
    include_str!("../THIRD_PARTY.md")
);

#[derive(Clone, Copy, Eq, PartialEq)]
enum Phase {
    Idle,
    Checking,
    Scheduled,
    Submitting,
}

struct Runtime {
    phase: Phase,
    locale: Locale,
    dry_run: bool,
    smoke_test: bool,
    action: Action,
    delay: Duration,
    timer: CountdownTimer,
    monotonic_origin: Instant,
    operation: Option<Receiver<Result<(), String>>>,
    exit_code: i32,
}

impl Runtime {
    fn sample(&self) -> ClockSample {
        ClockSample {
            steady: self.monotonic_origin.elapsed(),
            wall: SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap_or_default(),
        }
    }
}

fn set_language(ui: &AppWindow, state: &Runtime) {
    let s = strings(state.locale);
    ui.set_window_title(s.app_title.into());
    ui.set_heading(s.app_title.into());
    ui.set_subtitle(
        if state.dry_run {
            s.subtitle_dry
        } else {
            s.subtitle
        }
        .into(),
    );
    ui.set_language_label(s.language.into());
    ui.set_sleep_label(s.sleep.into());
    ui.set_shutdown_label(s.shutdown.into());
    ui.set_through_label(s.through.into());
    ui.set_hours_label(s.hours.into());
    ui.set_minutes_label(s.minutes.into());
    ui.set_preset_labels(ModelRc::new(VecModel::from(
        s.presets
            .into_iter()
            .map(SharedString::from)
            .collect::<Vec<_>>(),
    )));
    ui.set_footer(
        if state.dry_run {
            s.dry_footer
        } else {
            s.normal_footer
        }
        .into(),
    );
}

fn update_preview(ui: &AppWindow, state: &Runtime) {
    if state.phase != Phase::Idle {
        return;
    }
    let s = strings(state.locale);
    let delay = parse_delay(ui.get_hours().as_str(), ui.get_minutes().as_str());
    ui.set_main_enabled(delay.is_some());
    ui.set_main_label(s.start.into());
    ui.set_status(s.not_started.into());
    match delay {
        Some(delay) => {
            ui.set_countdown(format_remaining(delay).into());
            ui.set_detail(
                if ui.get_shutdown_selected() {
                    s.shutdown_detail
                } else {
                    s.sleep_detail
                }
                .into(),
            );
        }
        None => {
            ui.set_countdown("--:--:--".into());
            ui.set_detail(s.invalid.into());
        }
    }
}

fn spawn_operation(state: &mut Runtime, check_only: bool) {
    let (sender, receiver) = mpsc::channel();
    let action = state.action;
    let locale = state.locale;
    let dry_run = state.dry_run;
    std::thread::spawn(move || {
        let result = if dry_run {
            Ok(())
        } else if check_only {
            platform::check(action, locale)
        } else {
            platform::execute(action, locale)
        };
        let _ = sender.send(result);
    });
    state.operation = Some(receiver);
}

fn set_idle(ui: &AppWindow, state: &mut Runtime, status: &str) {
    state.timer.cancel();
    state.phase = Phase::Idle;
    state.operation = None;
    ui.set_controls_enabled(true);
    ui.set_status(status.into());
    update_preview(ui, state);
    ui.set_status(status.into());
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut dry_run = false;
    let mut smoke_test = false;
    let mut locale = Locale::En;
    let mut args = std::env::args().skip(1);
    while let Some(argument) = args.next() {
        match argument.as_str() {
            "--dry-run" => dry_run = true,
            "--smoke-test" => smoke_test = true,
            "--license" => {
                println!("{LICENSES}");
                return Ok(());
            }
            "--help" => {
                println!(
                    "Power Timer\n  --dry-run     Never call OS power APIs\n  --smoke-test  Run a 2-second GUI test (requires --dry-run)\n  --lang en|ru  Select the interface language\n  --license     Show license notices"
                );
                return Ok(());
            }
            "--lang" => match args.next().as_deref() {
                Some("en") => locale = Locale::En,
                Some("ru") => locale = Locale::Ru,
                value => {
                    eprintln!("Unknown language: {}", value.unwrap_or("<missing>"));
                    std::process::exit(2);
                }
            },
            other => {
                eprintln!("Unknown argument: {other}");
                std::process::exit(2);
            }
        }
    }
    if smoke_test && !dry_run {
        eprintln!("--smoke-test requires --dry-run");
        std::process::exit(2);
    }

    let ui = AppWindow::new()?;
    #[cfg(windows)]
    let tray = TrayIcon::new()?;
    let state = Rc::new(RefCell::new(Runtime {
        phase: Phase::Idle,
        locale,
        dry_run,
        smoke_test,
        action: Action::Sleep,
        delay: Duration::ZERO,
        timer: CountdownTimer::default(),
        monotonic_origin: Instant::now(),
        operation: None,
        exit_code: 0,
    }));
    set_language(&ui, &state.borrow());
    update_preview(&ui, &state.borrow());

    #[cfg(windows)]
    {
        let window = ui.as_weak();
        tray.on_show_window(move || {
            if let Some(window) = window.upgrade() {
                let _ = window.show();
            }
        });
        tray.on_exit_app(|| {
            let _ = slint::quit_event_loop();
        });
    }

    {
        let ui = ui.as_weak();
        let state = state.clone();
        ui.unwrap().on_input_changed(move || {
            if let Some(ui) = ui.upgrade() {
                update_preview(&ui, &state.borrow());
            }
        });
    }
    {
        let ui = ui.as_weak();
        let state = state.clone();
        ui.unwrap().on_select_action(move |shutdown| {
            if let Some(ui) = ui.upgrade() {
                ui.set_shutdown_selected(shutdown);
                state.borrow_mut().action = if shutdown {
                    Action::Shutdown
                } else {
                    Action::Sleep
                };
                update_preview(&ui, &state.borrow());
            }
        });
    }
    {
        let ui = ui.as_weak();
        let state = state.clone();
        ui.unwrap().on_select_preset(move |index| {
            if let Some(ui) = ui.upgrade() {
                let minutes = [15, 30, 60, 120].get(index as usize).copied().unwrap_or(30);
                ui.set_hours((minutes / 60).to_string().into());
                ui.set_minutes((minutes % 60).to_string().into());
                update_preview(&ui, &state.borrow());
            }
        });
    }
    {
        let ui = ui.as_weak();
        let state = state.clone();
        ui.unwrap().on_language_action(move || {
            if let Some(ui) = ui.upgrade() {
                let mut state = state.borrow_mut();
                if state.phase != Phase::Idle {
                    return;
                }
                state.locale = if state.locale == Locale::En {
                    Locale::Ru
                } else {
                    Locale::En
                };
                set_language(&ui, &state);
                update_preview(&ui, &state);
            }
        });
    }
    {
        let ui = ui.as_weak();
        let state = state.clone();
        ui.unwrap().on_escape_action(move || {
            if let Some(ui) = ui.upgrade() {
                let mut state = state.borrow_mut();
                if state.phase == Phase::Scheduled {
                    let message = strings(state.locale).cancelled;
                    set_idle(&ui, &mut state, message);
                }
            }
        });
    }
    {
        let ui = ui.as_weak();
        let state = state.clone();
        ui.unwrap().on_main_action(move || {
            if let Some(ui) = ui.upgrade() {
                let mut state = state.borrow_mut();
                let s = strings(state.locale);
                if state.phase == Phase::Scheduled {
                    set_idle(&ui, &mut state, s.cancelled);
                } else if state.phase == Phase::Idle {
                    let Some(delay) =
                        parse_delay(ui.get_hours().as_str(), ui.get_minutes().as_str())
                    else {
                        return;
                    };
                    state.delay = delay;
                    state.action = if ui.get_shutdown_selected() {
                        Action::Shutdown
                    } else {
                        Action::Sleep
                    };
                    state.phase = Phase::Checking;
                    ui.set_controls_enabled(false);
                    ui.set_main_enabled(false);
                    ui.set_status(s.checking.into());
                    ui.set_detail(s.checking_detail.into());
                    spawn_operation(&mut state, true);
                }
            }
        });
    }

    let poller = Timer::default();
    {
        let ui = ui.as_weak();
        let state = state.clone();
        poller.start(TimerMode::Repeated, Duration::from_millis(100), move || {
            let Some(ui) = ui.upgrade() else {
                return;
            };
            let mut state = state.borrow_mut();
            let s = strings(state.locale);
            if matches!(state.phase, Phase::Checking | Phase::Submitting) {
                let result = state
                    .operation
                    .as_ref()
                    .and_then(|receiver| receiver.try_recv().ok());
                if let Some(result) = result {
                    state.operation = None;
                    if let Err(error) = result {
                        state.exit_code = 1;
                        set_idle(&ui, &mut state, s.failed);
                        ui.set_detail(error.into());
                        if state.smoke_test {
                            let _ = slint::quit_event_loop();
                        }
                    } else if state.phase == Phase::Checking {
                        let sample = state.sample();
                        let action = state.action;
                        let delay = state.delay;
                        state.timer.start(action, delay, sample);
                        state.phase = Phase::Scheduled;
                        ui.set_main_enabled(true);
                        ui.set_main_label(s.cancel.into());
                        ui.set_status(s.started.into());
                        let expected = UNIX_EPOCH + state.timer.expected_at();
                        let local: DateTime<Local> = expected.into();
                        ui.set_detail(
                            format!(
                                "{} · {}",
                                if action == Action::Sleep {
                                    s.sleep
                                } else {
                                    s.shutdown
                                },
                                local.format("%d.%m, %H:%M:%S")
                            )
                            .into(),
                        );
                    } else {
                        let status = if state.dry_run {
                            s.finished
                        } else {
                            s.submitted
                        };
                        set_idle(&ui, &mut state, status);
                        if state.smoke_test {
                            let _ = slint::quit_event_loop();
                        }
                    }
                }
                return;
            }
            if state.phase != Phase::Scheduled {
                return;
            }
            let sample = state.sample();
            match state.timer.tick(sample) {
                Tick::Running => {
                    ui.set_countdown(format_remaining(state.timer.remaining(state.sample())).into())
                }
                Tick::Interrupted => {
                    set_idle(&ui, &mut state, s.interrupted);
                    ui.set_detail(s.interrupted_detail.into());
                }
                Tick::Due => {
                    state.phase = Phase::Submitting;
                    ui.set_countdown("00:00:00".into());
                    ui.set_status(s.sending.into());
                    ui.set_detail(s.unavailable_cancel.into());
                    ui.set_main_enabled(false);
                    spawn_operation(&mut state, false);
                }
                Tick::Idle => {}
            }
        });
    }

    if smoke_test {
        ui.set_hours("0".into());
        ui.set_minutes("0".into());
        let mut state = state.borrow_mut();
        state.delay = Duration::from_secs(2);
        state.phase = Phase::Checking;
        ui.set_controls_enabled(false);
        ui.set_main_enabled(false);
        spawn_operation(&mut state, true);
    }
    ui.run()?;
    std::process::exit(state.borrow().exit_code)
}
