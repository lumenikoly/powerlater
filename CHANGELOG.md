# Changelog

## 0.0.1

Initial release.

- Schedule sleep or shutdown from 1 minute to 24 hours.
- English and Russian interface, with English as the default.
- Minimal light interface with rounded controls, a large countdown, and selected presets.
- Quick presets, countdown, cancellation, and permission checks.
- Cancel the timer after a long pause or a significant clock change.
- Windows, macOS, and Linux (systemd) backends.
- Safe dry-run mode, automated tests, and GitHub Actions builds.

The timer runs only while the application and computer remain awake.
Closing the window cancels a pending timer. An already submitted system
request cannot be withdrawn. Save your work before scheduling shutdown.
