# Hext 0.12.0 verification

Validated on Windows 11 x64 during November 19-20, 2026. This release integrates authorization and assisted setup into the native chat while reusing the existing gate, provider adapters, task engine and encrypted store.

## Implemented behavior

- Host-created interactive messages cover concrete approvals, sensitive sharing, window-reading consent, assisted connection/configuration, user participation, recipe review, memory proposals and task resumption. The conversation and composer remain visible. Native button controls use the existing material, typography and springs.
- Allow/deny/cancel decisions resolve the component and record its state. Callback bindings include component, conversation, task, revision and the live approval nonce. Repeated or stale clicks cannot approve a following step. Provider text cannot create those bindings or issue native permission tickets.
- Public setup fields use the chat composer. Readiness checks resume the exact pending step and original goal through current permissions. Official account login, Windows choosers and platform authorization remain external when required. Provider verification preserves the setup component while invalidating old callbacks.
- Scrolling/navigation/collapse preserve a live request. Switching conversations pauses active work and revokes its authority. Encrypted checkpoints include bounded component/message context, including for an unsaved pending task. Restart restores inactive historical components and an explicit Resume action; old approval authority never survives restart.
- The three global permission modes, sensitive-result hold/release and native critical-operation protections remain effective. No second authorization system or provider-owned computer tools were introduced.

## Executed verification

| Check | Observed result |
| --- | --- |
| Native build and existing renderer | Compiled the C++17 x64 application; existing renderer checks completed with zero failures. |
| Existing personal/authorization controllers | Conversation/memory/configuration/adapter flows, three modes, mandatory protected consent and real sensitive-file hold/deny/release passed. |
| Interactive approval | Actual timer start/pause required separate inline approvals in Ask mode. Clicking the first component again did not approve the second request. |
| Deny and cancel | Prevented actual timer effects, resolved the native component, invalidated its buttons and stopped pending work. Denial offered a manual/adapted alternative in the conversation. |
| Navigation and visibility | History browsing, page changes and island collapse preserved the same live request. Controls outside the current chat could not execute. Drawing and hit-target clipping were checked independently. |
| Pause and expiry | Kept the original durable goal with no active ticket and exposed inline recovery. |
| Assisted configuration | Native test-adapter setup used the chat composer. Invalid input preserved the editor/waiting step; valid input resumed the original request through a fresh approval and started the real timer. The adapter is a local test fixture, not a simulated external login. |
| Provider verification reset | Preserved the inline setup and original goal while invalidating previous callback bindings. |
| Separate application process | Restored the encrypted conversation/pending context, refused the old approval, retained the original goal, requested missing setup, then required a newly issued approval before a real timer effect. The recovery fixture supplies a validated plan and does not pretend to be provider inference. |
| Actual Codex planning and native participation | Protected authenticated Codex calculated a SHA-256 result inside AppContainer, used the real result in an owned Win32 fixture, invoked its control, observed the result and completed the dependent timer goal. Zero agent-controller failures. |
| Independent real Codex review | Delegate approved a timer plus actual isolated note creation/readback without human confirmation, and refused or escalated an action contrary to the original explanation-only goal. No contrary timer started. |
| Installed application | Hext 0.12.0 is running and responds. Its executable matches the verified preview; native shortcut-message expansion, collapse/layer restoration and independent hide/show flows passed. |
| Presentation/protocol | Native DIB frames for approval, resolved/denied states, setup, pause and restart were inspected. A provider response saying permission was granted stayed plain text and created neither a component nor authority. Malformed/unsupported component metadata was refused. |

## Limits

Successful new Spotify OAuth/playback or a fresh third-party account login was not established by these fixtures. Those still need the user's service configuration/consent and actual readiness results. Claude was unavailable for account-backed validation. This release does not claim those integrations succeeded merely because their controls render.

Historical presentation is not authorization. After restart, tasks replan against current outcomes; UI scopes/snapshots, approval tickets, queued effects and cached package proposals are not reused. Recognized sensitive results held only in memory are not silently persisted or shared; recovery needs fresh checked observations. Cancellation cannot recall an external effect already accepted by Windows or a service.

The broader Windows 10/hardware coverage, incomplete sensitive/critical-content classification, unknown UI-invocation consent, AppContainer and external-platform limitations documented in [0.11.0](verification-0.11.0.md) and [General agent](agent.md) remain. Generated stores, workspaces, account diagnostics and private settings are excluded from Git and release archives.
