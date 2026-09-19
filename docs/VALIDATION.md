# Validation

All power-changing tests must use `--dry-run`. Never automate a real sleep or shutdown in CI.

Run locally:

```sh
cargo fmt --all -- --check
cargo test
cargo clippy --all-targets -- -D warnings
cargo build --release
cargo run --release -- --dry-run --smoke-test --lang en
cargo run --release -- --dry-run --smoke-test --lang ru
```

Unit tests cover interval parsing, one-shot expiry, cancellation after clock discontinuities, countdown formatting, and both localization catalogs. CI builds on Windows, macOS, and Linux. Real power actions, macOS permission prompts, signing, and notarization require manual validation.
