# Development

Use a Windows x64 MinGW-w64 environment with `g++`, `windres`, and PowerShell. C++17 is required. The application links Windows system libraries, including Direct2D, DirectWrite, Core Audio and Bluetooth. No Node.js, Electron, Python runtime, or .NET runtime is needed by Hext.exe.

```powershell
./app/build.ps1 -Preview
./scripts/test.ps1
./app/dist/Hext-preview.exe --verify-render
```

`-Preview` writes a separate executable so a running installation is preserved. Omit it only when the output executable is not in use. Preferences remain beside the executable in `hext.ini`; do not put a personal copy in a release ZIP.

## Verification

The console suite checks springs, desktop-region overlap, visibility, presentation, colors, wallpaper blur, hardware parsing, assistant subprocesses, audio device selection, and shortcuts. Shortcut registration tests use two hidden windows and temporary uncommon combinations; no keypress is injected. A restricted desktop sandbox may deny registration even when normal Windows execution permits it.

`--verify-render` exports native frames and exercises controls in a hidden window. Animation fixtures explicitly enable motion, regardless of a build runner's Windows preferences; reduced motion is also tested separately. Hardware and audio devices are read, but volume, default endpoints, and Bluetooth pairing are not changed. Unavailable-device, capture, and conflict images are explicit renderer fixtures. The optional `tests/visual-check.py` checks pixel alignment and requires Pillow and NumPy for development only.

`--verify-hardware` reads local sensors. `--verify-connection` checks the selected CLI without inference. `--record-motion` exports a 600-frame renderer sequence without recording the desktop. The runtime shortcut script operates only on Hext's own window after deployment; it briefly opens and closes the real panel. It queries non-sensitive state flags, uses the registered shortcut IDs, and waits for observable states instead of assuming fixed startup timings.

`app/tests/verify-desktop-recovery.ps1` performs a live desktop regression check. It temporarily minimizes applications, uses an owned window to cover and expose the island three times, and restores the previous applications afterward. It checks that normal compact presence returns without opening another app. It does not capture the screen or inject keys, and is run interactively rather than on CI. The hidden renderer separately checks a return that interrupts an exit fade.

## Releases

The Windows workflow builds and tests pushes and pull requests. A push to `main` publishes the current Hext version if that version is absent. It includes a clean x64 application ZIP, documentation, and checksums. Existing release assets are not silently overwritten. No audio engine or installer is downloaded or published by this workflow.

The GitHub Actions token is supplied by GitHub only at runtime. No repository secret or personal access token is required for the build. CI must be green before treating a published binary as validated. Broad Windows 10/11 hardware coverage is separate from passing software tests.
