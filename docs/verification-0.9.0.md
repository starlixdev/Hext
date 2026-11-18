# Hext 0.9.0 verification

Validated locally on Windows 11 with MinGW-w64 and the installed authenticated Codex CLI on November 18, 2026. These are observed results, not a claim of universal Windows automation or account coverage. Existing 0.8.x functionality remains covered by the regression suite and [previous integration report](verification-0.8.0.md).

| Component | Observed result |
| --- | --- |
| Native build and existing regressions | Passed the final native build, console tests for motion, springs, desktop overlap/recovery, presentation, colors/materials, hardware formatting, subprocesses, sound, shortcuts, and personal-assistant core. |
| Typed registry | Passed catalog/schema consistency, strict provider variants, invalid parameter/extra-field rejection, native dispatch revalidation, and unknown-tool rejection. |
| Permissions | Passed granted, denied, revoked, category/service/tool combinations, fresh approval after configuration, one-use tickets, and native-scoped Spotify policy on generic media controls. A model cannot supply the native target field. |
| Task continuation | The actual controller paused a dependent file task for a missing folder scope, authorized the fixture folder, requested fresh execution permission, and resumed the original request without another user message. Denial prevented file creation. |
| Real multi-turn Codex | Four provider turns planned three dependent operations: read an authorized fixture document, create a second document containing the actual returned marker, and start a two-minute timer. Verified exact file content, operation order, real timer state, and completed task status. |
| Real unsupported request | The authenticated provider handled an unavailable system task without dispatching an invented installer, command, or privileged action. |
| Provider lifecycle and isolation | Real Codex completed structured turns successfully. A separate real inference could not create its requested marker in the read-only provider workspace. The console protocol fixture also passed: a completed JSON stream released Hext's owned job promptly even when the child lingered. The fixture is not presented as a live provider test. |
| Skills | Passed typed recipe expansion and rejection of executable code, shell interpolation, recursive calls, unknown tools, oversized recipes, and invalid inputs. Expanded actions retain individual permission checks. |
| Native adapter extension | A controller fixture registered a capability/schema, exposed setup and a public configuration field, rejected invalid configuration without losing the goal, resumed through fresh approval, and started the actual isolated Hext timer. Disconnect invoked cleanup. New adapters defaulted to service confirmation despite a generally allowed category. This fixture is absent from normal runs. |
| Actual application discovery | Bounded registry/Start menu discovery found 127 eligible desktop targets on this computer. It launched and authorized none of them. |
| Actual text operations | Verified UTF-8 creation, content readback, no overwrite, filename/type/size bounds, and derived scope revocation. Controller/provider fixtures used generated files, not personal documents. |
| Persistent state | A separate Windows process restored the saved conversation, shared memory, language, category denial, and service/tool overrides. Existing chat isolation, rename/delete, reviewed memory, and data deletion tests passed. |
| Native Windows media inspection | WinRT inspection returned a current compatible session successfully. Spotify-filtered inspection returned no matching player. No playback or track metadata was read or changed by this test. Successful playback remains unverified. |
| Native interface | Both hidden native renderer/controller reports ended with zero failures. Resource list, resource permissions, setup continuation, file and skill states were exported from Hext's own DIB and visually inspected. Named controls have unique targets and valid bounds; no desktop capture was used. |
| Deployed application | Updated the real Hext executable to 0.9.0 while preserving existing settings/data. Its hash matches the tested preview, and the process responds. Live expansion/collapse, independent hide/show, topmost cleanup, and three coverage/exposure desktop recovery cycles passed. |

## Configuration and remaining limits

- Spotify named-track search and Web API playback still require official application/account configuration and the relevant platform access. Successful OAuth, renewal, account search, device transfer, and playback were not authenticated end to end here. The missing-connection continuation and policy/error branches were tested without inventing account data. Basic native Spotify control additionally needs an identifiable active player; none was present for this final check.
- Claude was not installed/authenticated on the validation computer; live Claude behavior remains unverified. Codex was exercised with a real authenticated provider, beyond subprocess fixtures.
- Browser search opens the default browser; Hext does not read or summarize the resulting page. App discovery does not cover every Store application or shortcut requiring arguments. Unknown services require a reviewed native adapter; no arbitrary MCP server, downloaded executable, or universal UI automation layer is claimed.
- Tasks resume after configuration within the running process. Pending execution is not persisted or replayed after quitting. Saved chats, shared preferences, permissions, and reviewed recipes remain persistent.
- This update did not reauthenticate Spotify or repeat every account-dependent integration recorded in the previous report. Windows 10, other device/DPI combinations, and accessibility workflows need additional coverage.
- Global shortcuts require Hext to be running. By default, `Ctrl+Alt+H` toggles normal desktop presence and `Ctrl+Alt+Space` expands/collapses the panel above applications. Existing shortcut customization is preserved; no automatic-startup feature is claimed.

See [development instructions](development.md) to reproduce the account-free checks and opt-in native/provider tests. Fixture files, databases, private configuration, and provider debug output are ignored and excluded from source and release packages.
