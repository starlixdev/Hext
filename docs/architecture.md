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

`personal-core.h` owns strict JSON validation, the permission gate, one-use approval tickets, encrypted SQLite store, bounded memory, and registered-asset validation. Its included `capability-registry.h` defines one discoverable catalog and generates validation/provider schemas from it. `personal-state.h` contains controller state. `personal-runtime.h` handles conversations, context construction, dispatch, cancellation, and host-authored receipts. `personal-ui.h` draws these flows using existing controls and springs, inside the same island. `personal-verification.h` exercises the actual controller in an isolated application instance. The task engine and native adapter contract are described in [Tasks and capabilities](tasks.md).

The flow is: native input → bounded context → restricted CLI → strict JSON envelope → validate entire batch → global permission gate → human or independent AI approval if needed → typed native executor → observed outcome. `authorization-policy.h` owns the three-mode gate and distinct one-use human/reviewer permits; `authorization-runtime.h` refines native risk, runs the separate restricted reviewer and holds recognized sensitive results before disclosure. AI review receives the original goal and exact action, never authority to approve future steps. Unknown commands never become unrestricted shell strings. The gate is evaluated again before dispatch. Permission revisions and conversation IDs invalidate old approvals. Failed actions stop the remaining batch. The capability registry remains internal; there is no capabilities settings list or granular permission persistence.

`chat-interaction.h` validates bounded presentation metadata; `chat-runtime.h` connects native interactive messages to the existing authorization/task handlers; `chat-ui.h` lays them out within the scrollable chat. Button IDs are unique to a component and bind conversation/task/revision/nonce. Their native callbacks cannot be created by provider output. Old button motion/bindings are discarded after resolution. The renderer clips both drawing and interactive bounds. Component arrival uses the existing spring engine; resolved components remain as history. Persistence stores presentation and task context, never live tickets or button bindings. Restored pending components are inactive; a host-owned Resume action replans with fresh checks.

Short Core Audio and timer operations run on the UI thread. Spotify HTTPS/OAuth and Windows WinRT media-session requests run on cancellable workers. Completion is associated with the initiating conversation. Canceling cannot recall external effects already accepted. Spotify results remain outside provider context. Ordinary cancellation does not join workers on the UI thread; shutdown waits for bounded I/O to finish.

`personal-net.h` uses fixed Spotify HTTPS hosts, disables redirects, validates OAuth state, and uses WinHTTP and Windows cryptography. `personal-media.h` calls GSMTC through the native WinRT ABI. Neither requires a web runtime. Existing audio, shortcuts, monitoring, desktop visibility, and island morph services remain intact.

To add an action, register its typed schema/capability and native executor, then add denied/allowed/failure checks. Validation and provider schema generation use that registry. New integrations must use the gate and treat external content as untrusted. Reviewed declarative skills compose existing tools without executable plugins. Dynamically loaded executors are not supported.

Persistence uses transactions, full SQLite synchronization, no WAL, secure deletion, a 64 MB page ceiling, and a one-megabyte payload limit. Each payload is DPAPI-encrypted. User drafts start transient; saved conversations autosave. Visual settings remain in `hext.ini`; private data lives under LocalAppData. Provider-owned authentication is not copied into Hext.

The general-agent layer reuses this pipeline: agent-process.h provides bounded owned jobs and AppContainer script execution; agent-desktop.h provides UI Automation in a short-lived MTA worker process; agent-web.h provides DNS-pinned public text research; agent-packages.h prepares reviewed WinGet targets; agent-runtime.h integrates scopes, receipts, recovery, and encrypted checkpoints; agent-ui.h uses the existing island controls. No second web UI or parallel permission engine is introduced. See [general agent architecture and boundaries](agent.md).

Conversational participation is implemented in conversation-policy.h and conversation-runtime.h. Native adapters may expose safe guidance and public-field descriptors; they do not implement service-specific chat parsers. The host gives these facts to the actual provider. capabilities.check, capabilities.configure, browser.open_url and internal task.control reuse the existing structured registry and dispatch pipeline. A suspended request stays distinct from helper actions, so an explanatory reply does not replace the original objective. Callback and polling paths continue to verify native operation ownership.
