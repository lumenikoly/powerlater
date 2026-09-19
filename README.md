# Power Timer

Power Timer is a small Rust desktop application for putting a computer to sleep or shutting it down after a delay. It uses [Slint](https://slint.dev/) for its cross-platform interface and native operating-system APIs for power actions.

## Features

- Sleep or shut down after 1 minute to 24 hours.
- Presets for 15 minutes, 30 minutes, 1 hour, and 2 hours.
- English and Russian interfaces.
- Cancels after a long event-loop pause, suspend, or material wall-clock change.
- `--dry-run` and `--smoke-test` never change the system power state.

The countdown exists only in the running application. Closing the window, logging out, rebooting, or a crash cancels it.

## Build and test

Install a current stable Rust toolchain, then run:

```sh
cargo build --release
cargo test
cargo clippy --all-targets -- -D warnings
```

The executable is `target/release/power-timer` (`power-timer.exe` on Windows).

```sh
cargo run --release
cargo run --release -- --lang ru
cargo run --release -- --dry-run --smoke-test
```

Linux builds require the development packages used by winit/OpenGL and runtime access to systemd-logind, `busctl`, and `systemctl`. macOS power control uses Apple Events. Windows uses WinAPI directly.

See [validation notes](docs/VALIDATION.md) and [release instructions](docs/RELEASING.md).

## License

MIT. Dependency notices are available through `power-timer --license` and in [THIRD_PARTY.md](THIRD_PARTY.md).
