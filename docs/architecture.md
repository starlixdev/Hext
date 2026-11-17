# Architecture

Hext uses one layered Win32 popup with a Direct2D DC render target and DirectWrite text. Expansion changes the same surface's dimensions and corner shape. It does not open a second window for the panel. A cached, blurred wallpaper sample supplies the translucent appearance; Hext does not continuously capture or blur the desktop.

`main.cpp` owns lifecycle, input, settings, desktop presentation, and renderer verification. `interface.h` and `sound-ui.h` draw controls. `motion.h` implements closed-form damped springs; changing a target preserves position and velocity. Settings and tabs use shorter transitions than the island's main morph. Reduced motion snaps positions to their targets.

## Desktop presence

Normal presence depends on whether the compact island's footprint is exposed. Visible, uncloaked, non-minimized windows suppress it only when an application surface is actually present in the overlapping area. Desktop focus takes precedence over stale window metadata during Show Desktop. The desktop host is refreshed before checking coverage, and returning during an exit fade replaces the stale frame with a fresh compact frame. A small foreground dialog elsewhere on the desktop does not hide the island.

The normal island stays above desktop icons and below application windows. Explicit keyboard expansion temporarily uses the topmost layer and restores that layer if Windows demotes it while manual mode is active. Collapsing removes it after the morph finishes and restores focus to the previous window when appropriate. The separate visibility shortcut pauses or resumes the whole island.

## Services

- `assistant.h` runs CLI probes and requests asynchronously, with a cancellable job.
- `hardware.h` reads native and optional sensor providers outside rendering.
- `sound.h` reads and edits Core Audio endpoints and enumerates Bluetooth devices.
- `shortcuts.h` acquires replacement global shortcuts before releasing working ones.

Only shortcut editing installs a temporary keyboard hook. It stops on save, cancellation, lost focus, page change, or timeout. No typed text is logged. Basic MSAA names, roles, values, and actions are exposed for custom controls; complete assistive-technology validation remains pending.

## Personal assistant components

`personal-core.h` owns strict JSON validation, the action catalog, permission gate, one-use approval tickets, encrypted SQLite store, bounded memory, and registered-asset validation. `personal-state.h` contains controller state. `personal-runtime.h` handles conversations, context construction, dispatch, cancellation, and host-authored receipts. `personal-ui.h` draws these flows using existing controls and springs, inside the same island. `personal-verification.h` exercises the actual controller in an isolated application instance.

The flow is: native input → bounded context → restricted CLI → strict JSON envelope → validate entire batch → permission gate → approval if needed → typed native executor → observed outcome. Unknown commands never become shell strings. The gate is evaluated again before dispatch. Permission revisions and conversation IDs invalidate old approvals. Failed actions stop the remaining batch.

Short Core Audio and timer operations run on the UI thread. Spotify HTTPS/OAuth and Windows WinRT media-session requests run on cancellable workers. Completion is associated with the initiating conversation. Canceling cannot recall external effects already accepted. Spotify results remain outside provider context. Ordinary cancellation does not join workers on the UI thread; shutdown waits for bounded I/O to finish.

`personal-net.h` uses fixed Spotify HTTPS hosts, disables redirects, validates OAuth state, and uses WinHTTP and Windows cryptography. `personal-media.h` calls GSMTC through the native WinRT ABI. Neither requires a web runtime. Existing audio, shortcuts, monitoring, desktop visibility, and island morph services remain intact.

To add an action, extend exact validation in `actionRequest`, the provider schema, permission mapping, concrete executor/receipt, and denied/allowed/failure tests. New integrations must use the gate and treat external content as untrusted. Dynamically loaded executors are not supported.

Persistence uses transactions, full SQLite synchronization, no WAL, secure deletion, a 64 MB page ceiling, and a one-megabyte payload limit. Each payload is DPAPI-encrypted. User drafts start transient; saved conversations autosave. Visual settings remain in `hext.ini`; private data lives under LocalAppData. Provider-owned authentication is not copied into Hext.
