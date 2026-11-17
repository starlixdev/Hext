# Architecture

Hext uses one layered Win32 popup with a Direct2D DC render target and DirectWrite text. Expansion changes the same surface's dimensions and corner shape. It does not open a second window for the panel. A cached, blurred wallpaper sample supplies the translucent appearance; Hext does not continuously capture or blur the desktop.

`main.cpp` owns lifecycle, input, settings, desktop presentation, and renderer verification. `interface.h` and `sound-ui.h` draw controls. `motion.h` implements closed-form damped springs; changing a target preserves position and velocity. Settings and tabs use shorter transitions than the island's main morph. Reduced motion snaps positions to their targets.

## Desktop presence

Normal presence depends on whether the compact island's footprint is exposed. Visible, uncloaked, non-minimized windows suppress it only when an application surface is actually present in the overlapping area. Desktop focus takes precedence over stale window metadata during Show Desktop. The desktop host is refreshed before checking coverage, and returning during an exit fade replaces the stale frame with a fresh compact frame. A small foreground dialog elsewhere on the desktop does not hide the island.

The normal island stays above desktop icons and below application windows. Explicit keyboard expansion temporarily uses the topmost layer and restores that layer if Windows demotes it while manual mode is active. Collapsing removes it after the morph finishes and restores focus to the previous window when appropriate. The separate visibility shortcut pauses or resumes the whole island.

## Services

- `assistant.h` runs CLI probes and requests asynchronously, with a cancellable job.
- `hardware.h` reads native and optional sensor providers outside rendering.
- `sound.h` reads and edits Core Audio endpoints and device-scoped APO configuration.
- `audio-addon.h` downloads one pinned audio installer, reports progress, verifies SHA-256, and separates downloading from installation.
- `shortcuts.h` acquires replacement global shortcuts before releasing working ones.

Only shortcut editing installs a temporary keyboard hook. It stops on save, cancellation, lost focus, page change, or timeout. No typed text is logged. Basic MSAA names, roles, values, and actions are exposed for custom controls; complete assistive-technology validation remains pending.
