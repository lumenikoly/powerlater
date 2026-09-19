# Releasing

1. Update the version in `Cargo.toml`, `Cargo.lock`, and `CHANGELOG.md`.
2. Run the checks from [VALIDATION.md](VALIDATION.md).
3. Run the manual **Build and release** GitHub Actions workflow.
4. Verify the three artifacts and the published release.

The workflow creates Windows x64, Linux x64, and macOS universal executables. Signing and notarization are not currently performed.
