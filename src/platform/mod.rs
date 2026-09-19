use crate::localization::Locale;
use crate::timer::Action;

#[cfg(target_os = "linux")]
mod linux;
#[cfg(target_os = "macos")]
mod macos;
#[cfg(windows)]
mod windows;

#[cfg(target_os = "linux")]
use linux as implementation;
#[cfg(target_os = "macos")]
use macos as implementation;
#[cfg(windows)]
use windows as implementation;

pub fn check(action: Action, locale: Locale) -> Result<(), String> {
    implementation::check(action, locale)
}

pub fn execute(action: Action, locale: Locale) -> Result<(), String> {
    implementation::execute(action, locale)
}
