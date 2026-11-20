# General Windows agent

Hext 0.10 adds general execution primitives to the existing task engine. The provider can compose a solution from environment inspection, public documentation, isolated PowerShell, authorized application controls, files, native Windows tools, and reviewed package proposals. A separate integration or command parser is not required for each goal.

Codex and Claude still return the same structured host protocol. Their own shell, browser, plugins, hooks, and MCP access remain disabled. Hext validates and executes the tools itself; changing providers does not move permission enforcement into a prompt.

## General capabilities

| Resource | Implemented access | Boundary |
| --- | --- | --- |
| Environment | `system.inspect`, existing installed-app discovery | Dependency/OS information and bounded registered desktop targets; not a private-document scan. |
| Isolated terminal | `terminal.run` | General Windows PowerShell in a per-conversation AppContainer, without network capabilities or personal-folder grants. No unrestricted fallback or automatic elevation. |
| Web research | `web.search`, `web.read` | Bing RSS search and public HTTPS text through the Windows curl executable. No browser cookies, credential forwarding, client configuration, or redirects. DNS is checked and curl connects to the validated IPv4 address while retaining TLS host verification. |
| Applications and browser UI | `computer.windows`, `computer.inspect`, `computer.act` | Windows UI Automation on scoped, visible, unelevated windows, governed by the global mode and native critical-action guards. Uses actual accessibility patterns; no screenshots, global keystroke injection, coordinate clicking, or hidden browser-debugging attachment. |
| User participation | `computer.wait` | Shows specific instructions and observes expected accessible text in an authorized window for up to two minutes. An observed condition resumes the same goal; it does not itself establish API account access or grant more permissions. |
| Software preparation | `packages.search`, `packages.inspect`, `packages.install` | Signed Microsoft App Installer, verified official WinGet community source, an exact package/version proposal and explicit installation approval. User scope only; normal hash/malware/UAC checks remain enabled. |

Existing native audio, timers, files, application launch, Windows media, Spotify, conversations, and shared preference memory remain available. Native integrations are preferable when their effects can be classified and verified more precisely than general UI actions.

## Terminal isolation

On the first permitted script, Hext creates a unique `Hext.Task.<conversation-id>` AppContainer. Only its own workspace receives that container SID's write access and a low-integrity label. Hext never changes permissions on a personal folder to make a script work. The child receives a small environment without provider credentials and no network capability SIDs. A private `Hext:` PowerShell drive supplies a usable workspace even when the drive root is inaccessible. `$env:HEXT_WORKSPACE` and `$env:TEMP` identify this task workspace.

Scripts may perform new calculations, parsing, transformations, and diagnostics permitted by that Windows token. They can create, edit, or delete files inside the isolated environment. An isolated script is not a mechanism for unrestricted host configuration. Use separately authorized file tools to exchange supported text with personal folders, UI tools for application interaction, and package tools for installation.

Each invocation has a 45-second wall limit, eight-process job limit, 384 MB aggregate job memory limit, bounded output, and job-owned cleanup. Cancellation terminates the owned job. AppContainer isolation can allow access to public system resources; it is not a promise that every host file is invisible. There is no filesystem disk quota. Hext limits retained workspace records to twenty; users should clear unneeded workspaces. External WinGet installers are separate from the isolated script environment and can have effects that cancellation cannot undo.

The workspace persists across script calls and application restarts. **Personalizar → Assistant → Dados → Limpar ambientes** deletes pending tasks and Hext-owned AppContainer profiles/data. Assistant data deletion includes these profiles. If an executor still owns files, Hext reports incomplete cleanup rather than silently claiming it succeeded.

## Application control

Listing window titles does not authorize reading their controls. First inspection requests permission for the selected window, with an explicit notice that content goes to the chosen AI provider. A scope binds the window handle to its PID and process creation time. It is discarded on conversation switching, revocation, and restart. Hext checks it again before execution.

Inspection returns at most 160 controls, bounded text, a snapshot ID, runtime element IDs, and supported patterns. Snapshots expire after three minutes. Password/protected controls are excluded; content marked offscreen is not used. Before acting, the worker reacquires the control and compares its observed state. A stale target is rejected without an alternative click.

Supported actions include invoke, value entry, toggle, selection, expand/collapse, and scrolling. Native-scoped benign actions (ordinary text entry, scrolling, expand/collapse, and a small set of recognized playback/search controls) follow the global mode. Unknown invocations and known financial, destructive, credential or critical-system contexts always ask the user. Ask mode also requires a concrete window scope; Delegate and Always attach an ordinary unelevated window only after authorized dispatch. Hext cannot infer all downstream consequences of arbitrary controls. Verified value/state changes and mere API acceptance are reported separately. An accepted invocation is not proof that a message was sent, an installation finished, or a task succeeded. The provider must observe subsequent state.

Protected/elevated windows, known command/administration hosts, Hext's own island, and known Spotify surfaces are excluded. Spotify catalog data remains outside general provider context; use the dedicated native capability. Browser automation works only for accessibility controls the browser exposes. It does not provide complete DOM access or support every site. Application-visible content can contain private data that is not labeled as a password; authorize windows accordingly.

Global authorization controls a particular access channel, not a semantic understanding of every downstream effect. Hext enforces the selected mode, window scopes, target checks, native high-risk consent and sensitive-result sharing consent. Its classifiers are conservative and incomplete. A separately launched third-party application may itself perform effects Hext cannot fully classify. Do not treat UI scopes as a per-file or per-service sandbox for that application's own behavior.

## Installation and configuration

WinGet is located through Microsoft's App Installer package identity and its executable signature is checked. Hext validates the configured `winget` source's type and official CDN URL before queries or installation. It cannot substitute a local manifest, alternate repository, custom installer arguments, hash bypass, forced elevation, or machine scope.

Inspection creates an opaque native proposal containing the ID, version, and package details. The approval view offers full details. Installation rechecks the exact package metadata and pins the version. Package license dialogs are not silently accepted. Missing access, agreements, unsupported user-scope installers, and UAC participation can require the user or prevent installation. Community manifest inclusion is not a guarantee that third-party software is appropriate or harmless.

Hext inspects installed state after the process returns. A failed, timed-out, or uncertain installation is not reported as completed, and it is not blindly retried. Successful installation does not imply that account login, application setup, or the original goal is complete. Those require subsequent observed steps.

## Durable tasks and recovery

Before dispatch, Hext writes a DPAPI-encrypted checkpoint containing the original goal, bounded task context, observations, authorized file-resource references, and the in-flight operation. Results update that record. If saving fails, no new operation is started. Up to twenty pending conversations are retained, with one pending task per conversation. A new goal in the same conversation replaces that conversation's pending goal.

Use **Pausar** to stop execution and retain a task. **Tarefas pendentes** reopens it without requiring the original request again. Closing Hext checkpoints active work. Saved chats and memory retain their existing independent behavior; a task checkpoint also contains the bounded context needed to resume an otherwise unsaved goal.

Resuming is explicit, not an automatic restart of external actions. Old approval tickets, UI scopes, snapshots, queued effects, and cached installation proposals are discarded. The provider plans again against actual outcomes. An in-flight effect interrupted by exit/crash is recorded as unknown and its identical dispatched write is not automatically repeated. Completed effects are not rolled back. New calls still pass current permissions. Recoverable, known failures may lead to a justified alternative; uncertain writes/installations must be inspected.

## Extensions and limits

Skills remain reviewed procedures composed from registered tools. They do not independently grant computer access or install new executors. Compiled native adapters extend the same registry and gate. Dynamic MCP server loading was considered but is not enabled: accepting a transport without an equivalent execution/credential boundary would let a provider or server bypass host controls. Future MCP adapters must preserve those boundaries.

This is a general foundation, not universal computer use. Accessible UI patterns, AppContainer compatibility, browser exposure, package availability and external authentication constrain which goals can be fulfilled. Voice input remains unimplemented. Real account integration, Windows 10 coverage, and additional applications require further validation. See the [current verification report](verification-0.11.0.md), [original agent verification](verification-0.10.0.md), and [development](development.md) for reproduction.

Public document transport permits at most 512 KiB per response and a bounded header/output allowance; extracted text returned to the provider remains limited to 16,000 UTF-8 bytes. Terminal and ordinary native process output retain their 64 KiB default limit. Public-address pinning, no redirects, no cookies/credentials, normal TLS and timeouts are unchanged. The separate document budget supports current official documentation pages without granting broader filesystem or network scope.
