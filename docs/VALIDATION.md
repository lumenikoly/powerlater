# Validation for 0.0.1

Local checks: 19 September 2026.

| Check | Result |
| --- | --- |
| Windows x64 Release build, Visual Studio 2026 / MSVC 19.51 | Passed |
| Windows CTest: timer, localization, CLI guards, embedded license, EN/RU GUI smoke tests | 7/7 passed |
| Windows UI inspection before visual redesign: English, Russian, language switching | Passed at the current desktop scale |
| Redesigned interface | Compiled; EN/RU GUI smoke tests passed; manual inspection stopped by user |
| Windows/Linux install layout | Passed; exactly one executable per platform, no documents or archives |
| Windows executable version metadata | Passed; version 0.0.1 |
| Linux core, Clang 18.1.3, ASan + UBSan + `-Werror` | 3/3 CTest suites passed |
| Linux x86_64 Release build, Ubuntu 24.04 WSL / GCC 13.3 | Passed |
| Linux CTest: timer, localization, processes, CLI guards, embedded license, EN/RU GUI via Xvfb | 8/8 passed |
| GitHub Actions YAML and shell syntax, manual trigger, unarchived uploads | Passed locally; remote execution not performed |

The timer suite covers all valid hours/minutes, invalid input, cancellation,
rescheduling, one-shot expiry, clock drift, and long pauses. Localization checks
cover the English default, Russian strings, explicit language snapshots, and
both complete translation catalogs. Process tests use a harmless fixture and
cover argument handling, exit status, bounded output, and timeouts.

The GUI smoke tests run with `--dry-run --smoke-test --lang en|ru`. They bypass
all operating-system power APIs and exit after a two-second countdown.
CLI tests reject smoke tests without dry-run and unsupported languages.

## Not verified locally

- macOS standalone universal compilation, embedded Info.plist, Automation permissions, and App Nap.
- Real sleep/shutdown actions, system inhibitors, and permission failures on
  physical machines. No real power action was triggered during validation.
- Other desktop scales, native Wayland, signing, and notarization.
- Remote GitHub Actions and the draft-release upload: the owner will publish
  the repository. The workflow must pass before publishing its release draft.

Windows builds report conversion warnings inside upstream FLTK headers; the
application builds successfully. Linux runtime support requires a graphical
session with systemd-logind; WSL build/tests do not validate real power control.
