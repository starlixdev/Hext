# Hext 0.11.0 verification

Validation on Windows 11 x64, November 19, 2026. This report distinguishes actual native/provider execution from protocol fixtures and checks that deliberately stop before high-risk effects.

## Changes

- Three global modes replace autonomy/category/service/tool overrides. Existing preferences migrate to Always Ask, preserving memory/language and the independent chat/connection records. All native executors, compiled adapters and expanded skill steps use the global gate.
- Delegate uses a separate invocation of the selected protected provider with the original goal, exact action and native execution boundaries. The planner's prose cannot approve an action. Human/reviewer permits are exact, expiring and consumed once; native protected operations require the human path in every mode.
- Settings have four tabs and no capabilities list. The internal capability registry, provider schemas, readiness discovery, inline setup, recipe import and native disconnect remain. Island rendering, springs, navigation and storage architecture are retained.
- Recognized sensitive tool results wait locally for explicit sharing consent before provider context or durable result data. Consent releases the held result without repeating its operation; denial discards it.

## Executed checks

| Check | Observed result |
| --- | --- |
| C++17/MinGW preview build | Built the native x64 application. |
| Console suite | Springs, desktop visibility/regions/presentation, colors/material, hardware parsing, CLI lifecycle, sound reads, strict tools, encrypted store/memory/chats, task engine and agent invariants passed. |
| Authorization unit test | Three effective modes; conservative migration; no legacy override persistence; native high-risk guards; forged/stale/mismatched/replayed human/reviewer permits rejected; strict review response schema; sensitive markers and ordinary task text distinguished. |
| Fresh process | Saved chat, memory, language, all three mode values and an interrupted task reloaded in a different process. The interrupted effect was not replayed. |
| Native permissions controller | Real timer effects required approval in Ask, denial prevented execution, dependent steps required their own exact approval, and Always executed a start/pause sequence automatically. |
| Protected installation | All three modes stopped at native human consent; no installer ran. This isolates authorization from dependency setup. It does not establish successful installation. |
| Sensitive output | A real generated text file with a credential marker was read. Its held data was absent from provider context and task checkpoint before sharing consent; denial discarded it, approval released its real result once. |
| Unavailable review | Fell back to a user dialog; no timer executed silently. |
| Actual independent Codex review | Authenticated/protected CLI approved a two-step timer plus isolated PowerShell note creation. Actual AppContainer file content was read back and matched the generated expected value; no user confirmation was used. The original goal remained intact. |
| Contrary action | The real reviewer refused or escalated a timer inconsistent with an explanation-only goal; no timer started. |
| Actual planner and Windows UI | Real Codex generated a SHA-256 calculation inside AppContainer, used its real result in an owned Win32 fixture, invoked Apply, inspected the resulting text and completed the dependent timer. The expected digest was independently calculated with BCrypt. Zero controller failures. |
| Native shortcuts and authentication failure | Windows registered temporary test combinations and detected conflicts without injected keypresses. An actual Spotify TLS request with an intentionally invalid token returned 401; no playback changed. |
| Personal controller and renderer | Existing conversation/memory/delete/setup/adapter flows and native renderer completed with zero failures. All mode frames exported; mode selection, typography, bounds and absence of capabilities navigation inspected visually. |
| Deployment | Hext 0.11.0 is running and responding; the deployed executable matches the verified preview. The actual island passed expansion above applications, collapse/layer restoration, and independent visibility/expansion shortcut-message flows. |

An initial reviewer uncertainty exposed missing host-boundary context: the reviewer did not know that TEMP is explicitly the isolated task workspace. The review now receives the actual native constraints. A separate false positive matching `sk-` inside `task-local` was fixed and covered by a regression test; review failure was never treated as approval.

## Remaining limits

- Independent review adds provider usage and latency. Each review is bounded to 45 seconds. Missing authentication, malformed responses, uncertainty, timeout and failed review ask the user instead of granting access.
- Claude review parsing and failure handling are covered by protocol tests. A real authenticated Claude installation was unavailable; successful account-backed Claude execution is unverified.
- Spotify OAuth/playback success needs a configured developer application, eligible account and active player. This change does not claim new account-backed Spotify validation. Normal Windows/service authentication and UAC remain in force.
- Critical UI/script and sensitive-content detection is conservative and incomplete; it is not universal semantic understanding or a guarantee of recognizing every private datum. Unknown general UI invocations still need human consent, even in Always mode. Unsupported unrestricted deletion/administration remains unsupported.
- No real payment, irreversible deletion, personal-file disclosure, critical system change or successful new software installation was performed as a test. Protected operations were checked before effect; native timer, generated file, AppContainer and owned-window effects are real.
- Voice and dynamic MCP loading remain unimplemented. Passing on this Windows 11 computer does not establish broad Windows 10/hardware/account coverage.
- The live cover/expose/return-to-desktop probe could not reach the desktop while this Windows session was locked (the lock-screen surface covered the island). Internal visibility/presentation tests passed; this is not reported as a passing live desktop recovery test.

Reproduction commands and boundaries are in [Development](development.md), [Assistant](assistant.md), and [General agent](agent.md). Generated databases, workspaces, CLI diagnostics and personal settings are excluded from the repository and release package.
