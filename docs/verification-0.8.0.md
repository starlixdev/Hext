# Hext 0.8.0 verification

The 0.8.1 follow-up gives close and cancel controls separate keyboard targets, clears stale notices, persists the last reopened chat, and preserves dirty chats when saving fails. Expanded controller verification covers a real storage payload-limit failure, canceled conversation switching, unique targets, and native frames for new-chat, memory-replacement review, delete-all confirmation, and the empty preference editor. Core integration results below also apply to 0.8.1; no provider or native action contract changed.

Validated on Windows 11 build 26200.9711, MinGW-w64, with Codex CLI 0.162.1, on November 17, 2026. This records observed local results; it is not certification of every Windows version, provider version, or account.

| Component | Observed result |
| --- | --- |
| Existing console regressions | Passed motion/springs, visibility, desktop coverage, presentation, colors/material, hardware formatting, subprocess/authentication cases, audio selection, and shortcut models. |
| Native shortcut registration | Passed temporary RegisterHotKey conflict, replacement, release, and cleanup checks without injected keys. |
| Running application | Updated the actual Hext executable to 0.8.0 while preserving settings. Live shortcut expansion/collapse, independent hide/show, and three desktop coverage/exposure cycles passed. The desktop test uses physical PerMonitorV2 coordinates to match Hext at non-default DPI. |
| Structured core | Passed unknown actions/fields, invalid types/ranges, autonomy/grant combinations and all category denials, one-use approval binding/replay/revocation, sensitive-volume confirmation, Spotify voice denial, and unsafe-asset rejection. |
| Private storage | Passed DPAPI payload encryption, transaction rollback, reopen, and deletion. A separate Windows process restored a conversation, shared preference, denied grant, and response language. |
| Memory | Passed normalization/upsert, explicit preference priority, provenance deletion, pinned preservation, 200-entry limit, shared context, and disabled-memory omission. |
| Actual controller | Passed timer approval and execution, denied/revoked dispatch, save/open/rename/delete with independent chats, persisted grants, and delete-all removal of chats, memory, assets, credentials, and visible state. |
| Real Codex | Passed version/options/authentication and Windows sandbox probe. A Portuguese request returned the correct timer action; no timer ran before approval, and approval started it. |
| Codex filesystem boundary | Native child could not read a generated canary outside the allowed workspace or create a file inside the read-only workspace. A real inference also failed to write a marker. |
| Windows audio | Actual executor wrote the existing mute state and exact existing integer volume, then read both back. Denied requests were refused. The user's audio state was preserved. |
| Application launch | Denied registered fixture launch did not run. Allowed launch executed a real native helper and produced its marker. Revoking its registered ID blocked subsequent execution. |
| Browser | Actual default-browser search launch accepted by Windows. Page load and contents were not inspected. |
| Windows media | Native WinRT session request completed. No current compatible player was available (ERROR_NOT_FOUND), so successful play/pause/skip remains unverified. No playback was changed. |
| Spotify authentication failure | Actual TLS request with an intentionally invalid token returned HTTP 401. Missing local credentials fail without reporting connected. No account or playback was simulated. |
| Spotify protocol primitives | Passed PKCE's RFC challenge vector, callback state mismatch, duplicate query parameter, invalid encoding, and denied-authorization rejection. |
| Native interface | Existing renderer and new controller renderer completed with zero failures. Settings, approvals, conversations, memory, and inline editor were visually inspected from native DIB exports. New controls have unique names and stay inside island bounds. Pixel checks passed existing button text, empty-state icon, send and stop alignment. |

## Requires user configuration or additional validation

- Spotify successful OAuth, token renewal against an account, track search, playback, transfer, and state confirmation require a registered Client ID, approved user, Spotify Premium where required, and an active device. These paths are implemented against the official API but were not authenticated end to end here.
- Claude was not installed/authenticated on the validation computer. Its restricted adapter and capability requirements are implemented; live Claude inference and account failures remain unverified. Mock subprocess tests are not a substitute for that validation.
- Generic text CLI execution is intentionally unavailable for the protected protocol.
- Native Windows media playback requires an active compatible player. Per-player command behavior remains unverified.
- Shell launching of an authorized document/folder uses the actual registered-path executor, but each user's default document handlers were not opened during validation. The executed fixture verifies application launch, not every file association.
- Windows 10 sandbox setup, other Windows builds, multiple DPI/monitor combinations, and screen-reader workflows need separate validation. The native island remains compatible with Windows 10/11 independently of a provider's sandbox availability.

Reproduce the checks using [Development](development.md). Generated databases, provider debug output, personal settings, and account data are ignored and excluded from releases. CI runs account-free tests; it does not claim live account integration coverage.
