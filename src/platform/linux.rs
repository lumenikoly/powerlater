use crate::localization::Locale;
use crate::timer::Action;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use std::time::{Duration, Instant};

fn msg(locale: Locale, en: &str, ru: &str) -> String {
    match locale {
        Locale::En => en,
        Locale::Ru => ru,
    }
    .to_owned()
}

fn executable(name: &str) -> Option<PathBuf> {
    ["/usr/bin", "/bin"]
        .iter()
        .map(|dir| Path::new(dir).join(name))
        .find(|p| p.is_file())
}

fn run(path: &Path, args: &[&str], timeout: Duration) -> Result<String, String> {
    let mut child = Command::new(path)
        .args(args)
        .stdin(Stdio::null())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .map_err(|error| error.to_string())?;
    let deadline = Instant::now() + timeout;
    loop {
        if child
            .try_wait()
            .map_err(|error| error.to_string())?
            .is_some()
        {
            let output = child
                .wait_with_output()
                .map_err(|error| error.to_string())?;
            if output.status.success() {
                let mut text = String::from_utf8_lossy(&output.stdout).into_owned();
                text.truncate(text.len().min(8192));
                return Ok(text);
            }
            let detail = String::from_utf8_lossy(&output.stderr);
            return Err(if detail.is_empty() {
                format!("exit code {}", output.status)
            } else {
                detail.into_owned()
            });
        }
        if Instant::now() >= deadline {
            let _ = child.kill();
            let _ = child.wait();
            return Err("system command timed out; the request will not be retried".into());
        }
        std::thread::sleep(Duration::from_millis(25));
    }
}

pub fn check(action: Action, locale: Locale) -> Result<(), String> {
    if unsafe { libc::geteuid() } == 0 {
        return Err(msg(
            locale,
            "Run the app as a regular user, not through sudo.",
            "Запустите приложение от обычного пользователя, не через sudo.",
        ));
    }
    let busctl = executable("busctl").ok_or_else(|| {
        msg(
            locale,
            "systemd-logind, busctl and systemctl are required.",
            "Нужны systemd-logind, busctl и systemctl.",
        )
    })?;
    executable("systemctl").ok_or_else(|| {
        msg(
            locale,
            "systemd-logind, busctl and systemctl are required.",
            "Нужны systemd-logind, busctl и systemctl.",
        )
    })?;
    let method = if action == Action::Sleep {
        "CanSuspend"
    } else {
        "CanPowerOff"
    };
    let output = run(
        &busctl,
        &[
            "--system",
            "--timeout=5s",
            "--allow-interactive-authorization=no",
            "call",
            "org.freedesktop.login1",
            "/org/freedesktop/login1",
            "org.freedesktop.login1.Manager",
            method,
        ],
        Duration::from_secs(6),
    )?;
    match output.trim() {
        "s \"yes\"" => Ok(()),
        "s \"challenge\"" => Err(msg(
            locale,
            "The system requires additional authorisation.",
            "Система требует дополнительную авторизацию.",
        )),
        value => Err(format!(
            "{} {value}",
            msg(locale, "The action is unavailable:", "Действие недоступно:")
        )),
    }
}

pub fn execute(action: Action, locale: Locale) -> Result<(), String> {
    check(action, locale)?;
    let systemctl = executable("systemctl").ok_or_else(|| "systemctl is unavailable".to_owned())?;
    let command = if action == Action::Sleep {
        "suspend"
    } else {
        "poweroff"
    };
    run(
        &systemctl,
        &["--no-ask-password", "--check-inhibitors=yes", command],
        Duration::from_secs(12),
    )
    .map(|_| ())
}
