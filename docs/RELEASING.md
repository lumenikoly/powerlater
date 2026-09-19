# Releasing Power Timer 0.0.1

The release workflow is manual. It builds and tests Windows x64, macOS universal, and Linux x64, then uploads one executable per platform. It does not create archives or upload documentation. The application and third-party license notices are available from each binary with `--license`.

## Before running Actions

1. Confirm that `CMakeLists.txt` contains the intended version and that `CHANGELOG.md` has a matching `## <version>` section.
2. Run the relevant local checks described in [VALIDATION.md](VALIDATION.md). Windows and Linux were checked locally; macOS still requires validation on macOS or in Actions.
3. Commit and push the source and workflow changes to GitHub.

## Run the workflow

1. Open **Actions → Build and release → Run workflow**.
2. Enter the version without the `v` prefix, for example `0.0.1`.
3. Leave `create_release` disabled for a build-only run, or enable it to create a draft release after all builds and tests pass.
4. Start the workflow and inspect all jobs.

The workflow validates the version against CMake and the changelog. Each build publishes a single executable artifact named like:

```text
PowerTimer-0.0.1-linux-x64
PowerTimer-0.0.1-windows-x64.exe
PowerTimer-0.0.1-macos-universal
```

The macOS artifact is a bare universal executable. Local build outputs are named `PowerTimer.exe` on Windows, `PowerTimer` on macOS, and `power-timer` on Linux; Actions adds the version and platform to their download names.

When `create_release` is enabled, the workflow creates tag `v<version>` and a **draft** GitHub release targeted at the workflow commit. The draft contains the three executable files only. Review the draft, verify the asset names and checks, then publish it manually. If the version tag already exists, validation stops the run before building.

## Download and run

Windows can run the `.exe` directly. On macOS and Linux, restore the executable bit after downloading:

```sh
chmod +x PowerTimer-0.0.1-macos-universal
chmod +x PowerTimer-0.0.1-linux-x64
```

Use `--help` for command-line options and `--license` for the embedded MIT and third-party notices. Real power actions remain subject to the operating system's permissions and policies.

Signing and notarization are not provided by this workflow. Do not describe the macOS binary as locally validated unless it has been run on macOS; the repository's current local validation does not establish that.
