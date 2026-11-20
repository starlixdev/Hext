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

## Personal assistant verification

The build fetches pinned nlohmann/json 3.12.0 and SQLite 3.54.1 sources through `scripts/dependencies.ps1`, checks SHA-256, and compiles SQLite with extension loading disabled. These build-time downloads stay in ignored `app/deps`; the executable needs no additional runtime. Licenses and source references are in `THIRD_PARTY_NOTICES.md`.

The console suite checks strict JSON, action parameters, all three global modes, conservative migration, human/reviewer permit binding/replay/revocation, Spotify voice denial, DPAPI encryption, transaction rollback, bounded memory, OAuth state, and a published PKCE vector. `restart-store-test` saves data including all three modes and reloads it in a separate Windows process. Default tests require no provider or Spotify account.

Use a hidden process and wait before reading the controller/renderer report:

```powershell
$check = Start-Process app/dist/Hext-preview.exe -ArgumentList '--verify-personal' -WindowStyle Hidden -PassThru
$check.WaitForExit()
Get-Content app/dist/personal-verification.txt
if ($check.ExitCode -ne 0) { throw 'Verification failed' }
```

This isolated instance exercises actual timer approval, denied/revoked requests, conversation save/open/rename/delete, shared/disabled memory, settings bounds, and named controls. It exports its own DIB surface without capturing the desktop. CI runs it alongside the existing renderer.

`--verify-permissions` adds account-free native tests for all three global modes and the four-tab interface. It denies/approves actual timer steps, runs an automatic timer sequence, checks mandatory installation consent without dispatching an installer, and reads a generated credential-marker file to verify that sensitive results are held before provider context/checkpoints, discarded on denial, or released once on approval. Native frames are `permissions-mode-0.bmp` through `permissions-mode-2.bmp`; the report is `authorization-verification.txt`. A test-only dependency-ready flag isolates package authorization from machine-specific WinGet setup; it does not simulate a successful installation.

`--verify-permissions-provider` additionally uses the real authenticated protected Codex CLI as the independent reviewer. It must authorize a timer plus creation/readback of a generated note inside the actual AppContainer without human confirmation, then deny or escalate a timer that contradicts the original request. This consumes provider usage. The primary planner is deliberately disabled in this fixture to isolate the review path; `--verify-agent-provider` separately verifies actual planning and native execution. Invalid/unavailable review falls back to human consent. Do not publish `authorization-review-debug.json` or generated private test directories. See [current results](verification-0.11.0.md).

Optional local tests must be reported separately:

- `--verify-personal-provider`: checks real Codex authentication/sandbox, exercises approved timer and dependent file/timer tasks in an isolated Hext instance, and checks an unsupported request. This consumes provider usage.
- `--verify-personal-native`: writes only the existing mute and exact integer volume back to the active output, checks denied audio dispatch, and inspects a Windows media session without playback. Add `provider` to combine both checks.
- For application/browser native checks, first build `g++ app/tests/native-action-child.cpp -o app/dist/native-action-child.exe -std=c++17 -O2 -static -mwindows`. The native verification then launches a registered copy of this fixture and opens one non-personal browser search. It does not inspect browser content or run arbitrary user applications.
- `personal-test.exe --live-auth-failure`: sends an intentionally invalid token to Spotify and checks a real 401, without account access or playback changes.
- `app/tests/verify-provider-boundary.ps1`: compiles a native child and verifies denied reads/writes against generated fixtures under Codex's actual Windows sandbox. No inference is used.
- `protected-provider-test.cpp`: optional real inference attempting to write a generated marker under the protected profile.

Never publish private fixture databases, `personal-provider-debug.json`, `task-provider-debug.json`, raw login output, or user settings. Passing error handling does not establish successful Spotify OAuth/playback. Claude needs a separately installed, authenticated CLI for full validation. See [validation results](verification-0.9.0.md).

The task console suite checks the shared tool schema, global authorization, native-scoped media policy, declarative recipe validation, duplicate/budget limits, actual UTF-8 text creation/readback, and a CLI lifecycle regression. Its generated child emits a completed structured turn but stays alive; Hext must decode completion and release its owned job without waiting thirty seconds. This is a process-protocol fixture, not a simulated provider integration.

The controller verifier also covers missing-resource suspension, configuration/approval resumption without another message, derived-file scope revocation, unsupported resources, setup cancellation, and native adapter registration/configuration. Its native adapter fixture starts the actual isolated Hext timer through the same global gate. The fixture is not installed in normal application runs.

`--verify-personal-provider` additionally performs a real multi-turn Codex task: read an authorized fixture file, create a second file from the returned content, and start a timer. It verifies content equality, ordering, timer state, and final task completion. Test approvals are limited to fixture file operations and the timer; the harness does not approve arbitrary actions from the provider. It also verifies a request that cannot be fulfilled. These calls consume provider usage.

`task-engine-test.exe --discover` performs actual bounded registry/Start menu discovery without launching or authorizing targets. `media-test.cpp` can be compiled with Windows runtime libraries to inspect the current session and a Spotify-filtered session without playback or track metadata. These machine-dependent checks are separate from account-free CI. Task screenshots are native DIB exports, not desktop recordings.

## General agent verification

The default console suite now includes agent-native-test (strict web/package inputs, permission migration, encrypted task snapshots, unknown-effect replay protection and bounded owned-child termination). The fresh-process restart test restores an interrupted task as paused, with its original goal and no execution replay.

Optional local checks, reported separately from CI:

- Compile app/tests/agent-sandbox-test.cpp with SQLite and -ladvapi32 -lcrypt32 -lbcrypt -luserenv -lole32. Run outside a restricted development sandbox: it creates a disposable AppContainer, writes/reads its own file across two invocations, and verifies denied private reads, external writes and network access (Windows socket error 10013).
- agent-native-test.exe --desktop uses only an owned Win32 fixture and verifies real accessibility observation, password exclusion, value entry, stale-control rejection, invocation and independent observation. It closes its fixture afterwards.
- agent-native-test.exe --web reads Microsoft documentation, searches Bing RSS and verifies the signed App Installer and official WinGet query path. It installs nothing.
- Hext-preview.exe --verify-agent runs the actual controller with an owned native fixture: window consent, action approval, denial, visible-condition continuation, task pause/restore and native DIB exports.
- Hext-preview.exe --verify-agent-provider additionally consumes real Codex usage. The provider generates a SHA-256 PowerShell calculation for a random input inside AppContainer, uses the actual hash in the fixture application, invokes its Apply control, observes the result and starts a real isolated timer. The verifier reads actual control text through cross-process Windows messages and checks an independent BCrypt digest. It never approves actions on personal windows or arbitrary packages.

Wait for each hidden verifier process before reading its report. Do not replace a running preview executable. UI Automation runs in a separate MTA worker process with a bounded job; the worker flag must be parsed from Windows command-line arguments, including quotes. These fixtures are tests of real native operations, not successful external account authentication. See docs/verification-0.10.0.md for observed results and remaining limits.
