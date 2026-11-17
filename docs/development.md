# Development

Use a Windows x64 MinGW-w64 environment with `g++`, `windres`, and PowerShell. C++17 is required. The application links Windows system libraries, including Direct2D, DirectWrite, Core Audio, WinHTTP, and BCrypt. No Node.js, Electron, Python runtime, or .NET runtime is needed by Hext.exe.

```powershell
./app/build.ps1 -Preview
./scripts/test.ps1
./app/dist/Hext-preview.exe --verify-render
```

`-Preview` writes a separate executable so a running installation is preserved. Omit it only when the output executable is not in use. Preferences remain beside the executable in `hext.ini`; do not put a personal copy in a release ZIP.

## Verification

The console suite checks springs, desktop-region overlap, visibility, presentation, colors, wallpaper blur, hardware parsing, assistant subprocesses, audio configuration, shortcuts, and SHA-256. Shortcut registration tests use two hidden windows and temporary uncommon combinations; no keypress is injected. A restricted desktop sandbox may deny registration even when normal Windows execution permits it.

`--verify-render` exports native frames and exercises controls in a hidden window. Animation fixtures explicitly enable motion, regardless of a build runner's Windows preferences; reduced motion is also tested separately. Hardware and audio devices are read, but volume, default endpoints, Bluetooth pairing, and live effects are not changed. Effect-enabled, unavailable-device, capture, and conflict images are explicit renderer fixtures. The optional `tests/visual-check.py` checks pixel alignment and requires Pillow and NumPy for development only.

`--verify-hardware` reads local sensors. `--verify-connection` checks the selected CLI without inference. `--record-motion` exports a 600-frame renderer sequence without recording the desktop. The runtime shortcut script operates only on Hext's own window after deployment; it briefly opens and closes the real panel.

Build `addon-test.exe` through `scripts/test.ps1`, then use `app/dist/addon-test.exe --download` to test the published add-on download and pinned digest. This test downloads but never runs the installer. Network and normal Windows desktop access may require running outside a restricted development sandbox.

## Releases

The Windows workflow builds and tests pushes and pull requests. A push to `main` prepares verified upstream add-on assets, creates the add-on release if absent, and publishes the current Hext version if that version is absent. It includes a clean x64 application ZIP, checksums, source archives, notices, and GPL texts. Existing release assets are not silently overwritten.

To change the add-on, update `addons/audio/upstream.json`, the pinned constants in `audio-addon.h`, and the source/installer hashes together. Inspect the upstream license and source changes. `scripts/prepare-audio-addon.ps1` downloads and verifies both archives before packaging them for redistribution. The native downloader uses the pinned Hext release URL rather than an unreviewed latest-release executable.

The GitHub Actions token is supplied by GitHub only at runtime. No repository secret or personal access token is required for the build. CI must be green before treating a published binary as validated. Physical-device audio processing and broad Windows 10/11 hardware coverage are separate from passing software tests.
